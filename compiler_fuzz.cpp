#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef CVM_SOURCE_DIR
#define CVM_SOURCE_DIR "."
#endif

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

namespace {

constexpr std::size_t kMaxInputBytes = 4095;
constexpr auto kCompilerTimeout = std::chrono::seconds(2);
constexpr std::uint64_t kDefaultSeed = 12648430;
const std::string kMutationAlphabet =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    "_+-*/%=!<>&|^~?:;,.()[]{}#\\\"' \t\n\r";

enum class RunOutcome { Completed, Crashed, TimedOut };

struct CorpusEntry {
    fs::path path;
    std::string label;
    std::string contents;
};

struct Options {
    std::size_t count = 100;
    std::uint64_t seed = kDefaultSeed;
    fs::path compiler;
    fs::path corpus = CVM_SOURCE_DIR;
    bool verbose = false;
};

std::string readFile(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read " + path.string());
    return std::string(std::istreambuf_iterator<char>(input),
                       std::istreambuf_iterator<char>());
}

std::vector<CorpusEntry> collectCorpus(const fs::path& root) {
    if (!fs::is_directory(root)) {
        throw std::runtime_error("corpus directory does not exist: " + root.string());
    }

    std::vector<CorpusEntry> corpus;
    std::error_code error;
    fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied,
                                        error);
    const fs::recursive_directory_iterator end;
    if (error) throw std::runtime_error("cannot scan corpus: " + error.message());

    while (it != end) {
        const fs::path path = it->path();
        const std::string name = path.filename().string();
        if (it->is_directory(error)) {
            if (name == "build" || name == ".git") it.disable_recursion_pending();
            error.clear();
            it.increment(error);
            if (error) error.clear();
            continue;
        }

        if (!error && it->is_regular_file(error) && path.extension() == ".pi") {
            const auto size = it->file_size(error);
            if (!error && size <= kMaxInputBytes) {
                CorpusEntry entry;
                entry.path = path;
                entry.label = fs::relative(path, root).generic_string();
                entry.contents = readFile(path);
                if (entry.contents.size() <= kMaxInputBytes) {
                    corpus.push_back(std::move(entry));
                }
            }
        }
        error.clear();
        it.increment(error);
        if (error) error.clear();
    }

    std::sort(corpus.begin(), corpus.end(), [](const CorpusEntry& left,
                                               const CorpusEntry& right) {
        return left.label < right.label;
    });
    return corpus;
}

std::string mutate(const std::string& source, std::mt19937_64& random) {
    std::string result = source;
    std::uniform_int_distribution<int> operationCount(1, 8);
    std::uniform_int_distribution<std::size_t> alphabetIndex(0,
                                                              kMutationAlphabet.size() - 1);
    const int operations = operationCount(random);

    for (int step = 0; step < operations; ++step) {
        std::vector<int> availableOperations = {0, 1, 2}; // insert, delete, replace
        if (result.size() >= kMaxInputBytes) availableOperations.erase(availableOperations.begin());
        if (result.empty()) availableOperations.erase(availableOperations.begin() + 1,
                                                       availableOperations.end());
        std::uniform_int_distribution<std::size_t> operationIndex(
            0, availableOperations.size() - 1);
        const int operation = availableOperations[operationIndex(random)];

        if (operation == 0) {
            std::uniform_int_distribution<std::size_t> position(0, result.size());
            result.insert(result.begin() + static_cast<std::ptrdiff_t>(position(random)),
                          kMutationAlphabet[alphabetIndex(random)]);
        } else if (operation == 1) {
            std::uniform_int_distribution<std::size_t> position(0, result.size() - 1);
            result.erase(position(random), 1);
        } else {
            std::uniform_int_distribution<std::size_t> position(0, result.size() - 1);
            result[position(random)] = kMutationAlphabet[alphabetIndex(random)];
        }
    }
    return result;
}

std::uint64_t hashContents(const std::string& contents) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char byte : contents) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

#ifdef _WIN32
std::string quoteWindowsArgument(const std::string& argument) {
    std::string quoted = "\"";
    std::size_t backslashes = 0;
    for (char ch : argument) {
        if (ch == '\\') {
            ++backslashes;
        } else if (ch == '"') {
            quoted.append(backslashes * 2 + 1, '\\');
            quoted += ch;
            backslashes = 0;
        } else {
            quoted.append(backslashes, '\\');
            quoted += ch;
            backslashes = 0;
        }
    }
    quoted.append(backslashes * 2, '\\');
    quoted += '"';
    return quoted;
}

RunOutcome runCompiler(const fs::path& compiler, const fs::path& input) {
    HANDLE nullDevice = CreateFileA("NUL", GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullDevice == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("cannot open NUL for compiler output");
    }

    STARTUPINFOA startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = nullDevice;
    startup.hStdError = nullDevice;
    PROCESS_INFORMATION process{};
    const std::string executable = compiler.string();
    std::string command = quoteWindowsArgument(executable) + " " +
                          quoteWindowsArgument(input.string()) + " --parse-only";
    std::vector<char> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back('\0');
    const BOOL created = CreateProcessA(executable.c_str(), mutableCommand.data(), nullptr,
                                        nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr,
                                        &startup, &process);
    CloseHandle(nullDevice);
    if (!created) throw std::runtime_error("cannot start compiler: " + executable);

    CloseHandle(process.hThread);
    const DWORD waitResult = WaitForSingleObject(process.hProcess, 2000);
    if (waitResult == WAIT_TIMEOUT) {
        TerminateProcess(process.hProcess, 1);
        WaitForSingleObject(process.hProcess, INFINITE);
        CloseHandle(process.hProcess);
        return RunOutcome::TimedOut;
    }
    if (waitResult != WAIT_OBJECT_0) {
        CloseHandle(process.hProcess);
        throw std::runtime_error("failed while waiting for compiler process");
    }

    DWORD exitCode = 0;
    if (!GetExitCodeProcess(process.hProcess, &exitCode)) {
        CloseHandle(process.hProcess);
        throw std::runtime_error("cannot read compiler exit status");
    }
    CloseHandle(process.hProcess);
    return exitCode >= 0xC0000000u ? RunOutcome::Crashed : RunOutcome::Completed;
}
#else
RunOutcome runCompiler(const fs::path& compiler, const fs::path& input) {
    const pid_t child = fork();
    if (child < 0) throw std::runtime_error("cannot create compiler process");
    if (child == 0) {
        const rlimit noCoreDump{0, 0};
        setrlimit(RLIMIT_CORE, &noCoreDump);
        const int nullDevice = open("/dev/null", O_WRONLY);
        if (nullDevice >= 0) {
            dup2(nullDevice, STDOUT_FILENO);
            dup2(nullDevice, STDERR_FILENO);
            close(nullDevice);
        }
        const std::string executable = compiler.string();
        const std::string source = input.string();
        char* const arguments[] = {const_cast<char*>(executable.c_str()),
                                   const_cast<char*>(source.c_str()),
                                   const_cast<char*>("--parse-only"), nullptr};
        execv(executable.c_str(), arguments);
        _exit(127);
    }

    const auto deadline = Clock::now() + kCompilerTimeout;
    while (true) {
        int status = 0;
        const pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            if (WIFSIGNALED(status)) return RunOutcome::Crashed;
            if (WIFEXITED(status)) return RunOutcome::Completed;
            return RunOutcome::Crashed;
        }
        if (result < 0) {
            if (errno == EINTR) continue;
            throw std::runtime_error("cannot wait for compiler process");
        }
        if (Clock::now() >= deadline) {
            kill(child, SIGKILL);
            while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
            return RunOutcome::TimedOut;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
#endif

std::uint64_t parseUnsigned(const std::string& text, const std::string& option) {
    std::size_t consumed = 0;
    unsigned long long value = 0;
    try {
        value = std::stoull(text, &consumed, 10);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid value for " + option + ": " + text);
    }
    if (consumed != text.size()) {
        throw std::runtime_error("invalid value for " + option + ": " + text);
    }
    return static_cast<std::uint64_t>(value);
}

void printUsage(const char* program) {
    std::cout << "Usage: " << program
              << " [--count N] [--seed N] [--compiler PATH] [--corpus DIR] [--verbose]\n"
              << "Defaults: --count 100 --seed " << kDefaultSeed
              << " --corpus " << CVM_SOURCE_DIR << "\n";
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            printUsage(argv[0]);
            std::exit(0);
        } else if (argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--count" || argument == "--seed" ||
                   argument == "--compiler" || argument == "--corpus") {
            if (index + 1 >= argc) {
                throw std::runtime_error("missing value for " + argument);
            }
            const std::string value = argv[++index];
            if (argument == "--count") {
                const std::uint64_t count = parseUnsigned(value, argument);
                if (count == 0 || count > static_cast<std::uint64_t>(SIZE_MAX)) {
                    throw std::runtime_error("--count must be a positive size");
                }
                options.count = static_cast<std::size_t>(count);
            } else if (argument == "--seed") {
                options.seed = parseUnsigned(value, argument);
            } else if (argument == "--compiler") {
                options.compiler = value;
            } else {
                options.corpus = value;
            }
        } else {
            throw std::runtime_error("unknown option: " + argument);
        }
    }
    return options;
}

} // namespace

int main(int argc, char** argv) {
    try {
        Options options = parseOptions(argc, argv);
        const fs::path executableDirectory = fs::absolute(argv[0]).parent_path();
        if (options.compiler.empty()) {
#ifdef _WIN32
            options.compiler = executableDirectory / "cvmpp.exe";
#else
            options.compiler = executableDirectory / "cvmpp";
#endif
        }
        options.compiler = fs::absolute(options.compiler).lexically_normal();
        if (!fs::is_regular_file(options.compiler)) {
            throw std::runtime_error("compiler executable not found: " +
                                     options.compiler.string());
        }
#ifndef _WIN32
        if (access(options.compiler.c_str(), X_OK) != 0) {
            throw std::runtime_error("compiler is not executable: " +
                                     options.compiler.string());
        }
#endif

        const std::vector<CorpusEntry> corpus = collectCorpus(options.corpus);
        if (corpus.empty()) {
            throw std::runtime_error("no .pi corpus files under " + options.corpus.string());
        }

        const fs::path buildDirectory = executableDirectory;
        const fs::path failuresDirectory = buildDirectory / "fuzz_failures";
        fs::create_directories(failuresDirectory);

#ifdef _WIN32
        const auto processId = static_cast<unsigned long>(GetCurrentProcessId());
#else
        const auto processId = static_cast<unsigned long>(getpid());
#endif
        const fs::path workDirectory = buildDirectory / ".compiler_fuzz_work" /
            ("run_" + std::to_string(processId) + "_" + std::to_string(options.seed));
        fs::create_directories(workDirectory);

        std::mt19937_64 random(options.seed);
        std::uniform_int_distribution<std::size_t> corpusIndex(0, corpus.size() - 1);
        std::size_t completed = 0;
        std::size_t crashes = 0;
        std::size_t timeouts = 0;

        std::cout << "seed=" << options.seed << " count=" << options.count
                  << " corpus=" << corpus.size() << " timeout=2s\n";

        for (std::size_t caseIndex = 0; caseIndex < options.count; ++caseIndex) {
            const CorpusEntry& source = corpus[corpusIndex(random)];
            const std::string generated = mutate(source.contents, random);
            if (generated.size() >= 4096) {
                throw std::runtime_error("internal error: generated input reached 4 KB");
            }

            std::ostringstream inputName;
            inputName << "input_" << std::setw(6) << std::setfill('0')
                      << (caseIndex + 1) << ".pi";
            const fs::path generatedPath = workDirectory / inputName.str();
            {
                std::ofstream output(generatedPath, std::ios::binary);
                output.write(generated.data(), static_cast<std::streamsize>(generated.size()));
                if (!output) throw std::runtime_error("cannot write generated input");
            }

            const RunOutcome outcome = runCompiler(options.compiler, generatedPath);
            if (outcome == RunOutcome::Completed) {
                ++completed;
            } else {
                const bool crashed = outcome == RunOutcome::Crashed;
                if (crashed) ++crashes;
                else ++timeouts;

                std::ostringstream failureName;
                failureName << "seed_" << options.seed << "_case_" << std::setw(6)
                            << std::setfill('0') << (caseIndex + 1)
                            << (crashed ? "_crash.pi" : "_timeout.pi");
                const fs::path savedPath = failuresDirectory / failureName.str();
                fs::copy_file(generatedPath, savedPath,
                              fs::copy_options::overwrite_existing);
                std::cout << (crashed ? "crash" : "timeout") << " in case "
                          << (caseIndex + 1) << " (" << source.label
                          << "), saved " << savedPath.string() << "\n";
            }

            if (options.verbose) {
                std::cout << "case " << (caseIndex + 1) << " source=" << source.label
                          << " hash=" << std::hex << std::setw(16) << std::setfill('0')
                          << hashContents(generated) << std::dec << std::setfill(' ')
                          << "\n";
            }
        }

        std::error_code cleanupError;
        fs::remove_all(workDirectory, cleanupError);
        std::cout << "finished: " << completed << " completed, " << crashes
                  << " crashes, " << timeouts << " timeouts\n";
        return (crashes == 0 && timeouts == 0) ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "compiler_fuzz: " << error.what() << "\n";
        return 2;
    }
}
