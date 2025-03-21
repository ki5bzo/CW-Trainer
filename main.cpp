#include <SFML/Audio.hpp>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <thread>
#include <chrono>
#include <limits>
#include <cctype>
#include <string>
#include <fstream>
#include <algorithm>
#include <random>
#include <cstring>
#include <cerrno>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <sys/select.h>
#include <sstream>

// A global clearScreen used in the top‐level menu:
void globalClearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

namespace MorseModule {

// --- getch() helper ---
#ifdef _WIN32
  #include <conio.h>
#else
char getch() {
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

// Global Morse code maps and other globals
std::map<char, std::string> morseCode = {
    {'A', ".-"},   {'B', "-..."},
    {'C', "-.-."}, {'D', "-.."},
    {'E', "."},    {'F', "..-."},
    {'G', "--."},  {'H', "...."},
    {'I', ".."},   {'J', ".---"},
    {'K', "-.-"},  {'L', ".-.."},
    {'M', "--"},   {'N', "-."},
    {'O', "---"},  {'P', ".--."},
    {'Q', "--.-"}, {'R', ".-."},
    {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"},
    {'W', ".--"},  {'X', "-..-"},
    {'Y', "-.--"}, {'Z', "--.."},
    {'0', "-----"},{'1', ".----"},
    {'2', "..---"},{'3', "...--"},
    {'4', "....-"},{'5', "....."},
    {'6', "-...."},{'7', "--..."},
    {'8', "---.."},{'9', "----."}
};

static void addPunctuation() {
    morseCode['.'] = ".-.-.-";  
    morseCode[','] = "--..--";  
    morseCode['?'] = "..--..";  
    morseCode['!'] = "-.-.--";  
    morseCode['-'] = "-....-";  
    morseCode['/'] = "-..-.";   
    morseCode['('] = "-.--.";   
    morseCode[')'] = "-.--.-";  
    morseCode[':'] = "---...";  
    morseCode[';'] = "-.-.-.";  
    morseCode['='] = "-...-";   
    morseCode['+'] = ".-.-.";   
    morseCode['\"'] = ".-..-."; 
    morseCode['\''] = ".----."; 
    morseCode['&'] = ".-...";   
    morseCode['_'] = "..--.-";  
    morseCode['@'] = ".--.-.";  
}

std::map<std::string, std::string> prosigns = {
    {"AR", "AR"},
    {"AS", "AS"},
    {"BK", "BK"},
    {"BT", "BT"},
    {"KA", "KA"},
    {"KN", "KN"}, 
    {"CL", "CL"},
    {"CQ", "CQ"},
    {"K",  "K"},
    {"R",  "R"},
    {"SK", "SK"},
    {"VE", "VE"}
};

std::vector<char> letters;
std::vector<char> numbers;

static const std::vector<char> punctuationChars = {
    '.', ',', '?', '!', '-', '/', '(', ')',
    ':', ';', '=', '+', '\"', '\'', '&', '_', '@'
};

static const std::vector<std::vector<char>> letterGroups = {
    {'E','I','S','H'},
    {'T','M','O'},
    {'A','U','V','N','D','B'},
    {'F','L','G','W','Q','Y'},
    {'K','R','P','X'},
    {'C','J','Z'}
};

// clearScreen (for Morse module)
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Play a beep using SFML (requires linking to SFML libraries)
void playBeep(float frequency, float durationSeconds) {
    const int sampleRate = 44100;
    const int numSamples = static_cast<int>(sampleRate * durationSeconds);
    short* samples = new short[numSamples];

    for (int i = 0; i < numSamples; ++i) {
        samples[i] = static_cast<short>(
            32767 * sin(2.0 * 3.14159 * frequency * i / sampleRate)
        );
    }

    sf::SoundBuffer buffer;
    if (!buffer.loadFromSamples(samples, numSamples, 1, sampleRate)) {
        std::cerr << "Failed to load audio buffer.\n";
    }

    sf::Sound sound;
    sound.setBuffer(buffer);
    sound.play();

    std::this_thread::sleep_for(std::chrono::milliseconds(
        static_cast<int>(durationSeconds * 1000.0f)
    ));
    delete[] samples;
}

void playMorseCode(const std::string& text, float pitch, int wpm, int effectiveWpm) {
    float unitDuration       = 1200.0f / wpm;        
    float farnsworthDuration = 1200.0f / effectiveWpm; 

    float intraCharSpace = unitDuration;
    float interCharSpace = farnsworthDuration * 3; 
    float interWordSpace = farnsworthDuration * 7;  

    for (char c : text) {
        char upperC = static_cast<char>(toupper(c));
        auto it = morseCode.find(upperC);
        if (it != morseCode.end()) {
            const std::string& pattern = it->second;
            for (char symbol : pattern) {
                if (symbol == '.') {
                    playBeep(pitch, unitDuration / 1000.f);
                } else if (symbol == '-') {
                    playBeep(pitch, (3 * unitDuration) / 1000.f);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    static_cast<int>(intraCharSpace)
                ));
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(interCharSpace)
            ));
        }
        else if (upperC == ' ') {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(interWordSpace)
            ));
        }
    }
}

std::map<std::string, int> loadMissStats(const std::string &filename) {
    std::map<std::string, int> stats;
    std::ifstream fin(filename);
    if (fin) {
        std::string item;
        int missCount;
        while (fin >> item >> missCount) {
            stats[item] = missCount;
        }
        fin.close();
    }
    return stats;
}

void saveMissStats(const std::map<std::string, int> &stats, const std::string &filename) {
    std::ofstream fout(filename);
    if (fout) {
        for (auto &entry : stats) {
            fout << entry.first << " " << entry.second << "\n";
        }
        fout.close();
    }
}

// --------------------
// Morse Module Modes
// --------------------
void runQuizMode(float pitch, int wpm, int effectiveWpm) {
    srand(static_cast<unsigned int>(time(nullptr)));
    while (true) {
        clearScreen();
        std::cout << "Choose quiz mode:\n"
                  << "1: Letters\n"
                  << "2: Numbers\n"
                  << "3: Mixed (letters + numbers)\n"
                  << "4: Prosigns\n"
                  << "5: Punctuation\n"
                  << "6: Exit quiz mode\n"
                  << "Enter your choice (1-6) ";
        int choice;
        std::cin >> choice;
        if (!std::cin || choice == 6) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        if (choice < 1 || choice > 6) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Invalid choice. Press ENTER to try again.\n";
            std::cin.get();
            continue;
        }
        clearScreen();
        std::cout << "How many questions in this session? ";
        int numQuestions;
        std::cin >> numQuestions;
        while (!std::cin || numQuestions <= 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a valid positive integer: ";
            std::cin >> numQuestions;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::vector<std::string> questionPool;
        if (choice == 1) {
            for (char letter : letters) questionPool.push_back(std::string(1, letter));
        } else if (choice == 2) {
            for (char number : numbers) questionPool.push_back(std::string(1, number));
        } else if (choice == 3) {
            for (char letter : letters) questionPool.push_back(std::string(1, letter));
            for (char number : numbers) questionPool.push_back(std::string(1, number));
        } else if (choice == 4) {
            for (auto &p : prosigns) questionPool.push_back(p.first);
        } else if (choice == 5) {
            for (char punc : punctuationChars) questionPool.push_back(std::string(1, punc));
        }

        if (numQuestions > static_cast<int>(questionPool.size())) {
            std::cout << "Warning: Only " << questionPool.size()
                      << " unique items available. Some duplicates may occur.\n";
            std::cin.get();
        }

        std::map<std::string, int> totalAttempts;
        std::map<std::string, int> correctAnswers;
        std::set<std::string> usedQuestions;

        for (int i = 0; i < numQuestions; ++i) {
            clearScreen();
            std::string question;
            do {
                question = questionPool[rand() % questionPool.size()];
            } while (usedQuestions.find(question) != usedQuestions.end() &&
                     usedQuestions.size() < questionPool.size());
            usedQuestions.insert(question);
            totalAttempts[question]++;

            std::cout << "Question " << (i + 1) << " of " << numQuestions << ":\n\n";
            if (choice == 4 && prosigns.find(question) != prosigns.end()) {
                playMorseCode(prosigns[question], pitch, wpm, effectiveWpm);
            } else {
                playMorseCode(question, pitch, wpm, effectiveWpm);
            }
            std::string userInput;
            if (choice == 4) {
                std::cout << "\nType your answer: ";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::getline(std::cin, userInput);
            } else {
                char userChar = tolower(getch());
                userInput = std::string(1, userChar);
            }
            std::string questionLower = question;
            std::string userLower     = userInput;
            for (char &c : questionLower) c = static_cast<char>(std::tolower(c));
            for (char &c : userLower)     c = static_cast<char>(std::tolower(c));
            if (userLower == questionLower) {
                correctAnswers[question]++;
            }
        }

        clearScreen();
        std::cout << "Quiz complete! Here are your results:\n\n";
        std::string mostMissedChar = "";
        int maxMisses = 0;
        for (auto &entry : totalAttempts) {
            int attempts = entry.second;
            int correct  = correctAnswers[entry.first];
            int misses   = attempts - correct;
            std::cout << "Item: " << entry.first
                      << " | Asked: " << attempts
                      << " | Correct: " << correct
                      << " | Missed: " << misses << "\n";
            if (misses > maxMisses) {
                maxMisses = misses;
                mostMissedChar = entry.first;
            }
        }
        if (!mostMissedChar.empty() && maxMisses > 0) {
            std::cout << "\nYour most missed character was: " << mostMissedChar 
                      << " (Missed " << maxMisses << " times)\n";
        } else {
            std::cout << "\nGreat job! You didn't miss any characters.\n";
        }
        std::cout << "\nPress ENTER to return to the quiz menu...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cin.get();
    }
}

void runPenAndPaperMode(float pitch, int wpm, int effectiveWpm) {
    clearScreen();
    int choice = 0;
    while (true) {
        std::cout << "Choose question type for this session:\n"
                  << "1. Letters only\n"
                  << "2. Numbers only\n"
                  << "3. Mix of letters + numbers\n"
                  << "4. Words\n"
                  << "5. Prosigns\n"
                  << "6. Punctuation\n"
                  << "7. Exit Pen and Paper Mode\n"
                  << "Enter your choice (1-7) ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 7) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        if (std::cin && choice >= 1 && choice <= 6) {
            break;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please try again.\n\n";
    }

    // ------------------------------------
    // Only ask "How many questions?" if NOT #4 (Words)
    // ------------------------------------
    int numQuestions = 0;
    if (choice != 4) {
        clearScreen();
        std::cout << "How many questions in this session? ";
        while (!(std::cin >> numQuestions) || numQuestions <= 0) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Please enter a valid positive number: ";
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    // ------------------------------------

    bool playAgain = true;
    while (playAgain) {
        clearScreen();
        std::vector<std::string> questionPool;

        if (choice == 1) {
            // Letters
            for (char letter : letters) 
                questionPool.push_back(std::string(1, letter));

        } else if (choice == 2) {
            // Numbers
            for (char number : numbers) 
                questionPool.push_back(std::string(1, number));

        } else if (choice == 3) {
            // Letters + Numbers
            for (char letter : letters) 
                questionPool.push_back(std::string(1, letter));
            for (char number : numbers) 
                questionPool.push_back(std::string(1, number));

        } else if (choice == 4) {
            // Words
            std::ifstream infile("wordlist");
            if (!infile) {
                std::cerr << "Error: Could not open file 'wordlist'.\n";
            } else {
                std::vector<std::string> words;
                std::string line;
                while (std::getline(infile, line)) {
                    if (!line.empty())
                        words.push_back(line);
                }
                infile.close();

                if (words.empty()) {
                    std::cout << "No words found in the file.\n";
                } else {
                    int letterCount = 0;
                    std::cout << "Enter the number of letters each word should have: ";
                    while (!(std::cin >> letterCount) || letterCount <= 0) {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Invalid input. Please enter a positive integer: ";
                    }
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                    // Filter words
                    std::vector<std::string> filtered;
                    for (const auto &word : words) {
                        if (static_cast<int>(word.size()) == letterCount)
                            filtered.push_back(word);
                    }
                    if (filtered.empty()) {
                        std::cout << "No words with exactly " 
                                  << letterCount << " letters were found.\n";
                    } else {
                        // Now ask how many words they want to study:
                        int numWords;
                        std::cout << "How many words do you want to study? ";
                        while (!(std::cin >> numWords) || numWords <= 0) {
                            std::cin.clear();
                            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                            std::cout << "Invalid input. Please enter a positive integer: ";
                        }
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

                        if (numWords > static_cast<int>(filtered.size())) {
                            std::cout << "Only " << filtered.size() << " words available with " 
                                      << letterCount << " letters. Using all available words.\n";
                            numWords = filtered.size();
                        }
                        // Shuffle the filtered words:
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::shuffle(filtered.begin(), filtered.end(), gen);

                        // Add them to the question pool
                        for (int i = 0; i < numWords; i++) {
                            questionPool.push_back(filtered[i]);
                        }
                    }
                }
            }

        } else if (choice == 5) {
            // Prosigns
            for (auto &p : prosigns) {
                questionPool.push_back(p.first);
            }

        } else if (choice == 6) {
            // Punctuation
            for (char punc : punctuationChars) {
                questionPool.push_back(std::string(1, punc));
            }
        }

        // If user chose 1/2/3/5/6 but typed in a large numQuestions:
        if (choice != 4 && 
            numQuestions > static_cast<int>(questionPool.size())) {
            std::cout << "Warning: Only " << questionPool.size()
                      << " unique items available. Some duplicates may occur.\n";
            std::cin.get();
        }

        std::set<std::string> usedQuestions;
        std::vector<std::string> correctAnswers;

        // If user picked #4, how many "questions" do we do?
        // We'll simply treat each “word” in the questionPool as one question:
        int totalToPlay = (choice == 4) 
            ? static_cast<int>(questionPool.size())
            : numQuestions;

        for (int i = 0; i < totalToPlay; ++i) {
            std::string question;
            if (!questionPool.empty()) {
                do {
                    question = questionPool[rand() % questionPool.size()];
                } while (
                    usedQuestions.find(question) != usedQuestions.end() &&
                    usedQuestions.size() < questionPool.size()
                );
                usedQuestions.insert(question);
                correctAnswers.push_back(question);

                if (prosigns.find(question) != prosigns.end()) {
                    playMorseCode(prosigns[question], pitch, wpm, effectiveWpm);
                } else {
                    playMorseCode(question, pitch, wpm, effectiveWpm);
                }
            }
        }

        clearScreen();
        std::cout << "Pen-and-paper session complete!\n\n"
                  << "Here are the answers:\n\n";
        for (size_t i = 0; i < correctAnswers.size(); ++i) {
            std::cout << (i + 1) << ". " << correctAnswers[i] << "\n";
        }

        std::cout << "\nWould you like to play again with the same settings? (y/n): ";
        char response;
        std::cin >> response;
        if (std::tolower(response) != 'y') {
            playAgain = false;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    clearScreen();
    std::cout << "Returning to the main menu...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}


void runSingleCharacterMode(float pitch, int wpm, int effectiveWpm) {
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (true) {
        clearScreen();
        std::cout << "Single Character Morse Playback Mode\n"
                  << "-------------------------------------\n"
                  << "Type one or two letters (or known prosign, e.g. AR)\n"
                  << "Press ENTER on empty line to Exit.\n\n"
                  << "Input: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }
        if (input.empty()) {
            break;
        }
        for (char &ch : input) {
            ch = static_cast<char>(std::toupper(ch));
        }
        bool didPlay = false;
        if (input.size() == 1) {
            char c = input[0];
            auto it = morseCode.find(c);
            if (it != morseCode.end()) {
                playMorseCode(std::string(1, c), pitch, wpm, effectiveWpm);
                std::cout << "\nPlayed character: " << c << "\n";
                didPlay = true;
            } else {
                std::cout << "\nNo Morse mapping for '" << c << "'\n";
            }
        } else {
            auto pit = prosigns.find(input);
            if (pit != prosigns.end()) {
                playMorseCode(pit->second, pitch, wpm, effectiveWpm);
                std::cout << "\nPlayed prosign: " << input << "\n";
                didPlay = true;
            } else {
                std::cout << "\nNot a recognized prosign. Will attempt to play each char individually...\n";
                for (char c : input) {
                    auto it = morseCode.find(c);
                    if (it != morseCode.end()) {
                        playMorseCode(std::string(1, c), pitch, wpm, effectiveWpm);
                        std::cout << "Played: " << c << "\n";
                        didPlay = true;
                    } else {
                        std::cout << "No Morse mapping for: " << c << "\n";
                    }
                }
            }
        }
        if (didPlay) {
            std::cout << "\nPress ENTER to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    clearScreen();
    std::cout << "Returning to the main menu...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void runLessonsMode(float pitch, int wpm, int effectiveWpm) {
    clearScreen();
    std::cout << "Welcome to Lesson Mode (Progressive Learning)\n\n";
    std::cout << "In each lesson, you'll practice a small group of letters.\n"
              << "We'll quiz you, and if you pass, you move to the next group.\n\n";
    std::vector<std::string> missedAllLessons;
    for (size_t lessonIndex = 0; lessonIndex < letterGroups.size(); ++lessonIndex) {
        clearScreen();
        std::cout << "Lesson " << (lessonIndex + 1) 
                  << " of " << letterGroups.size() << "\n\n";
        std::cout << "This lesson covers: ";
        for (char c : letterGroups[lessonIndex]) {
            std::cout << static_cast<char>(std::toupper(c)) << " ";
        }
        std::cout << "\nPress ENTER to begin the quiz.\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::vector<std::string> questionPool;
        for (char c : letterGroups[lessonIndex]) {
            questionPool.push_back(std::string(1, c));
        }
        int numQuestions = 25;
        int correctCount = 0;
        std::set<std::string> usedThisLesson;
        srand(static_cast<unsigned int>(time(nullptr)));
        for (int i = 0; i < numQuestions; ++i) {
            clearScreen();
            std::cout << "Lesson " << (lessonIndex + 1) << "/"
                      << letterGroups.size()
                      << " | Question " << (i + 1)
                      << " of " << numQuestions << "\n\n";
            std::string question;
            do {
                question = questionPool[rand() % questionPool.size()];
            } while (usedThisLesson.find(question) != usedThisLesson.end() &&
                     usedThisLesson.size() < questionPool.size());
            usedThisLesson.insert(question);
            playMorseCode(question, pitch, wpm, effectiveWpm);
            std::cout << "\nEnter your single-character answer: ";
            std::cout.flush();
            char userChar = tolower(getch());
            std::string userInput(1, userChar);
            std::cout << userInput << "\n";
            std::string questionLower = question;
            for (char &c : questionLower) c = static_cast<char>(std::tolower(c));
            for (char &c : userInput)     c = static_cast<char>(std::tolower(c));
            if (userInput == questionLower) {
                std::cout << "Correct!\n";
                correctCount++;
            } else {
                std::cout << "Incorrect. Correct answer was: " << question << "\n";
                missedAllLessons.push_back(question);
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        double scorePercent = 100.0 * correctCount / numQuestions;
        clearScreen();
        std::cout << "Lesson " << (lessonIndex + 1) << " complete!\n\n"
                  << "You scored " << correctCount << " out of " << numQuestions
                  << " (" << scorePercent << "%)\n";
        if (scorePercent < 80.0) {
            std::cout << "You might want to repeat this lesson before moving on.\n";
            std::cout << "Press ENTER to retry the same lesson...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            lessonIndex--;
        } else {
            std::cout << "Good job! Press ENTER to move to the next lesson...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    clearScreen();
    if (missedAllLessons.empty()) {
        std::cout << "You have completed all lessons!\n\n"
                  << "Great job! You didn't miss any characters.\n";
    } else {
        std::cout << "You have completed all lessons!\n\n"
                  << "Here are the characters you missed:\n\n";
        for (size_t i = 0; i < missedAllLessons.size(); ++i) {
            std::cout << (i + 1) << ". " << missedAllLessons[i] << "\n";
        }
    }
    std::cout << "\nReturning to the main menu...\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void runSpeedChallengeMode(float pitch, int wpm, int effectiveWpm) {
    clearScreen();
    std::cout << "Speed Challenge Mode!\n";
    //std::cin.clear();
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int selection = 0;
    while (true) {
        std::cout << "What do you want to study?\n"
                  << "1. Letters\n"
                  << "2. Numbers\n"
                  << "3. Punctuation\n"
                  << "4. Prosigns\n"
                  << "Enter your choice (1-4): ";
        std::cin >> selection;
        if (std::cin && selection >= 1 && selection <= 4) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please try again.\n\n";
    }
    clearScreen();
    std::cout << "How many total questions? ";
    int numQuestions;
    std::cin >> numQuestions;
    while (!std::cin || numQuestions <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid positive integer: ";
        std::cin >> numQuestions;
    }
    float timeLimitSeconds = 0.0f;
    std::cout << "Enter max seconds to respond per question: ";
    std::cin >> timeLimitSeconds;
    while (!std::cin || timeLimitSeconds <= 0.0f) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid positive number of seconds: ";
        std::cin >> timeLimitSeconds;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::vector<std::string> questionPool;
    switch (selection) {
        case 1:
            for (char letter : letters) {
                questionPool.push_back(std::string(1, letter));
            }
            break;
        case 2:
            for (char number : numbers) {
                questionPool.push_back(std::string(1, number));
            }
            break;
        case 3:
            for (char punc : punctuationChars) {
                questionPool.push_back(std::string(1, punc));
            }
            break;
        case 4:
            for (auto &p : prosigns) {
                questionPool.push_back(p.first);
            }
            break;
    }
    int correctCount  = 0;
    int timedOutCount = 0;  
    srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 1; i <= numQuestions; ++i) {
        clearScreen();
        std::cout << "Speed Challenge - Question " << i
                  << " of " << numQuestions << "\n\n";
        std::string question = questionPool[rand() % questionPool.size()];
        if (selection == 4 && prosigns.find(question) != prosigns.end()) {
            playMorseCode(prosigns[question], pitch, wpm, effectiveWpm);
        } else {
            playMorseCode(question, pitch, wpm, effectiveWpm);
        }
        auto startTime = std::chrono::high_resolution_clock::now();
        std::cout << "\nPress your single-character answer before "
                  << timeLimitSeconds << " seconds pass!\n";
        std::cout.flush();
        char userChar = tolower(getch());
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                             endTime - startTime).count() / 1000.0;
        std::string userInput(1, userChar);
        std::cout << "\nYou typed: " << userInput << "\n"
                  << "Time taken: " << elapsed << " seconds\n";
        bool isTimedOut = (elapsed > timeLimitSeconds);
        if (isTimedOut) {
            std::cout << "TIME'S UP!\n";
            timedOutCount++;
        }
        std::string questionLower = question;
        for (char &c : questionLower) c = static_cast<char>(std::tolower(c));
        for (char &c : userInput)    c = static_cast<char>(std::tolower(c));
        if (userInput == questionLower && !isTimedOut) {
            correctCount++;
            std::cout << "Correct!\n";
        }
        else if (userInput == questionLower && isTimedOut) {
            std::cout << "You got the right answer, but you're out of time!\n";
        } else {
            std::cout << "Wrong answer. The correct answer was: " << question << "\n";
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    clearScreen();
    std::cout << "Speed Challenge Complete!\n\n"
              << "Questions: " << numQuestions << "\n"
              << "Correct within time limit: " << correctCount << "\n"
              << "Timed out: " << timedOutCount << "\n";
    int missedCount = numQuestions - correctCount;
    std::cout << "Missed: " << missedCount << "\n";
    double percentage = (100.0 * correctCount) / numQuestions;
    std::cout << "Accuracy: " << percentage << "%\n\n";
    std::cout << "Press ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

void runSpacedRepetitionQuiz(float pitch, int wpm, int effectiveWpm) {
    clearScreen();
    //std::cout << "Spaced-Repetition Quiz Mode\n\n";
    //std::cin.clear();
    //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    int selection = 0;
    while (true) {
        std::cout << "Which characters would you like to study?\n"
                  << "1. Letters\n"
                  << "2. Numbers\n"
                  << "3. Punctuation\n"
                  << "4. Prosigns\n"
                  << "Enter your choice (1-4) ";
        std::cin >> selection;
        if (std::cin && selection >= 1 && selection <= 4) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Invalid choice. Please try again.\n\n";
    }
    clearScreen();
    std::cout << "How many total questions? ";
    int numQuestions;
    std::cin >> numQuestions;
    while (!std::cin || numQuestions <= 0) {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::cout << "Please enter a valid positive integer: ";
        std::cin >> numQuestions;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::map<std::string, int> persistentMisses = loadMissStats("misses.txt");
    std::vector<std::string> masterPool;
    switch (selection) {
        case 1:
            for (char letter : letters) {
                masterPool.push_back(std::string(1, letter));
            }
            break;
        case 2:
            for (char number : numbers) {
                masterPool.push_back(std::string(1, number));
            }
            break;
        case 3:
            for (char punc : punctuationChars) {
                masterPool.push_back(std::string(1, punc));
            }
            break;
        case 4:
            for (auto &p : prosigns) {
                masterPool.push_back(p.first);
            }
            break;
    }
    for (auto &item : masterPool) {
        if (persistentMisses.find(item) == persistentMisses.end()) {
            persistentMisses[item] = 0;
        }
    }
    srand(static_cast<unsigned int>(time(nullptr)));
    int quizMissCount = 0;
    for (int q = 1; q <= numQuestions; ++q) {
        clearScreen();
        std::cout << "Spaced-Repetition Quiz - Question " << q
                  << " of " << numQuestions << "\n\n";
        long long totalWeight = 0;
        for (auto &entry : persistentMisses) {
            if (std::find(masterPool.begin(), masterPool.end(), entry.first) == masterPool.end()) {
                continue;
            }
            totalWeight += (1 + entry.second);
        }
        long long r = rand() % totalWeight;
        std::string question;
        long long cumulative = 0;
        for (auto &entry : persistentMisses) {
            if (std::find(masterPool.begin(), masterPool.end(), entry.first) == masterPool.end()) {
                continue;
            }
            cumulative += (1 + entry.second);
            if (r < cumulative) {
                question = entry.first;
                break;
            }
        }
        if (question.empty() && !masterPool.empty()) {
            question = masterPool[rand() % masterPool.size()];
        }
        if (selection == 4 && prosigns.find(question) != prosigns.end()) {
            playMorseCode(prosigns[question], pitch, wpm, effectiveWpm);
        } else {
            playMorseCode(question, pitch, wpm, effectiveWpm);
        }
        std::cout << "\nEnter your single-character answer: ";
        std::cout.flush();
        char userChar = tolower(getch());
        std::string userInput(1, userChar);
        std::cout << userInput << "\n";
        std::string questionLower = question;
        for (char &c : questionLower) c = static_cast<char>(std::tolower(c));
        for (char &c : userInput)    c = static_cast<char>(std::tolower(c));
        if (questionLower == userInput) {
            std::cout << "Correct!\n";
        } else {
            std::cout << "Incorrect. Correct answer was: " << question << "\n";
            persistentMisses[question]++;
            quizMissCount++;
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    clearScreen();
    std::cout << "Spaced-Repetition Quiz Complete!\n\n";
    if (quizMissCount > 0) {
        std::cout << "You missed " << quizMissCount << " time"
                  << (quizMissCount == 1 ? "" : "s")
                  << " this session.\n\n";
    } else {
        std::cout << "Great job! You didn't miss any this session.\n\n";
    }
    std::cout << "Saving updated stats to 'misses.txt'...\n";
    saveMissStats(persistentMisses, "misses.txt");
    std::cout << "\nAll-time characters missed (per 'misses.txt'):\n";
    bool anyMissedOverall = false;
    for (auto &entry : persistentMisses) {
        if (entry.second > 0) {
            std::cout << "  " << entry.first << " missed " << entry.second << " times total.\n";
            anyMissedOverall = true;
        }
    }
    if (!anyMissedOverall) {
        std::cout << "  None missed overall!\n";
    }
    std::cout << "\nPress ENTER to continue...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// --- Wrap the original Morse10.cpp main loop as a function ---
void morseMain() {
    addPunctuation();
    for (char c = 'A'; c <= 'Z'; ++c) {
        letters.push_back(c);
    }
    for (char c = '0'; c <= '9'; ++c) {
        numbers.push_back(c);
    }
    float pitch = 800.0f;
    int wpm = 20;
    int effectiveWpm = 10;
    clearScreen();
    std::cout << "Practice Morse Code\n";
    std::cout << "Enter pitch (Hz), e.g. 800: ";
    std::cin >> pitch;
    std::cout << "Enter character speed (WPM), e.g. 20: ";
    std::cin >> wpm;
    std::cout << "Enter Farnsworth speed (WPM), e.g. 10: ";
    std::cin >> effectiveWpm;
    if (effectiveWpm > wpm) {
        std::cerr << "Farnsworth speed cannot exceed character speed. Setting Farnsworth to WPM.\n";
        effectiveWpm = wpm;
    }
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    while (true) {
        clearScreen();
        std::cout << "Choose an option:\n"
                  << "1: Convert text to Morse Code\n"
                  << "2: Morse Code Quiz Mode\n"
                  << "3: Pen-and-Paper Mode\n"
                  << "4: Single-Character Playback Mode\n"
                  << "5: Spaced-Repetition Quiz\n"
                  << "6: Lessons Mode (Progressive)\n"
                  << "7: Speed Challenge Mode\n"
                  << "8: Return to Main Menu\n"
                  << "Enter your choice (1-8) ";
                  
        int choice=0;
        std::cin >> choice;
        if (!std::cin) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (choice == 8) {
            break;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 1) {
            clearScreen();
            std::cout << "Enter text to convert to Morse Code:\n";
            std::string text;
            std::getline(std::cin, text);
            playMorseCode(text, pitch, wpm, effectiveWpm);
            std::cout << "\nPress ENTER to continue...";
            std::cin.get();
        } else if (choice == 2) {
            runQuizMode(pitch, wpm, effectiveWpm);
        } else if (choice == 3) {
            runPenAndPaperMode(pitch, wpm, effectiveWpm);
        } else if (choice == 4) {
            runSingleCharacterMode(pitch, wpm, effectiveWpm);
        } else if (choice == 5) {
            runSpacedRepetitionQuiz(pitch, wpm, effectiveWpm);
        } else if (choice == 6) {
            runLessonsMode(pitch, wpm, effectiveWpm);
        } else if (choice == 7) {
            runSpeedChallengeMode(pitch, wpm, effectiveWpm);
        } else {
            std::cout << "Invalid choice. Try again.\n";
        }
    }
}

} // end namespace MorseModule

// ------------------------------------------------------------
// WinKeyer Module (formerly main.cpp)
// ------------------------------------------------------------
namespace WinKeyerModule {

volatile bool running = true;
int currentWPM = 20;
int speedPotMinWpm = 5;
int speedPotMaxWpm = 35;
bool useBufferedSpeedChange = false;
bool internalSpeakerOn = true;
unsigned char pinCfg = 0x0F; // internal speaker on
static const int POLL_USEC = 10000; // 10 ms poll
static const double GRACE_SEC = 0.05; // 50 ms

void sigint_handler(int) {
    running = false;
}

void clearScreen() {
    std::cout << "\033[2J\033[H";
    std::cout.flush();
}

bool writeBytes(int fd, const unsigned char* data, size_t length) {
    ssize_t written = 0;
    while (written < static_cast<ssize_t>(length)) {
        ssize_t w = write(fd, data + written, length - written);
        if (w < 0) {
            std::cerr << "Error writing to serial port: " << strerror(errno) << "\n";
            return false;
        }
        written += w;
    }
    return true;
}

bool writeCmd(int fd, unsigned char c1, unsigned char c2 = 0, bool twoBytes = false) {
    unsigned char buf[2] = { c1, c2 };
    return writeBytes(fd, buf, twoBytes ? 2 : 1);
}

void delay_ms(unsigned int ms) {
    usleep(ms * 1000);
}

void updateHeader(int wpm) {
    std::cout << "\033[s\033[1;60HWPM: " << wpm << "   \033[u";
    std::cout.flush();
}

bool initWinkeyer(int fd) {
    unsigned char hostCmd[2] = {0x00, 0x02};
    if (!writeBytes(fd, hostCmd, 2)) return false;
    delay_ms(300);
    unsigned char ver;
    int n = read(fd, &ver, 1);
    if (n == 1)
        std::cout << "WinKeyer firmware version: " << (int)ver << "\n";
    else
        std::cout << "No firmware version byte read...\n";
    unsigned char winkMode = 0xC4;
    if (!writeCmd(fd, 0x0E, winkMode, true)) return false;
    delay_ms(200);
    if (useBufferedSpeedChange) {
        if (!writeCmd(fd, 0x1C, static_cast<unsigned char>(currentWPM), true)) return false;
    } else {
        if (!writeCmd(fd, 0x02, static_cast<unsigned char>(currentWPM), true)) return false;
    }
    delay_ms(200);
    if (!writeCmd(fd, 0x09, pinCfg, true)) return false;
    delay_ms(200);
    std::cout << "WinKeyer initialized.\n";
    return true;
}

struct termios orig_stdin;
void setNonCanonicalStdin() {
    tcgetattr(STDIN_FILENO, &orig_stdin);
    struct termios newt = orig_stdin;
    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
}
void restoreStdin() {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_stdin);
}

std::vector<std::string> winLoadWordlist(const std::string &filename) {
    std::vector<std::string> words;
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Could not open '" << filename << "'\n";
        return words;
    }
    std::string line;
    while (std::getline(fin, line)) {
        if (!line.empty())
            words.push_back(line);
    }
    return words;
}

void practiceGameLoop(int fd) {
    clearScreen();
    std::cout << "===== PRACTICE MENU ======\n"
              << "1) Letters (A-Z)\n"
              << "2) Numbers (0-9)\n"
              << "3) Punctuation\n"
              << "4) Words\n"
              << "0) Back\n"
              << "Enter option: ";
    std::string line;
    std::getline(std::cin, line);
    if (line == "0") return;
    std::vector<std::string> practiceItems;
    if (line == "1") {
        for (char c = 'A'; c <= 'Z'; ++c)
            practiceItems.push_back(std::string(1, c));
    } else if (line == "2") {
        for (char c = '0'; c <= '9'; ++c)
            practiceItems.push_back(std::string(1, c));
    } else if (line == "3") {
        practiceItems = {".", ",", "?", "/", "=", "-", ";"};
    } else if (line == "4") {
        practiceItems = winLoadWordlist("wordlist");
        if (practiceItems.empty()) {
            std::cout << "No words found. Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        std::cout << "Please specify the desired word length." << std::flush;
        std::getline(std::cin, line);
        int letterCount = 0;
        try {
            letterCount = std::stoi(line);
        } catch (...) {
            letterCount = 0;
        }
        if (letterCount <= 0) {
            std::cout << "Invalid letter count. Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        std::vector<std::string> filteredWords;
        for (auto &word : practiceItems) {
            if (word.size() == static_cast<size_t>(letterCount) &&
                std::all_of(word.begin(), word.end(), [](char c){ return std::isalpha(c); }))
            {
                filteredWords.push_back(word);
            }
        }
        if (filteredWords.empty()) {
            std::cout << "No words with " << letterCount << " letters found. Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return;
        }
        practiceItems = filteredWords;
        
    } else {
        std::cout << "Invalid choice. Press Enter...\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    // REPLACE WITH:
std::random_device rd;
std::mt19937 rng(rd());
std::shuffle(practiceItems.begin(), practiceItems.end(), rng);

int numItems = static_cast<int>(practiceItems.size());
    int itemIndex = 0, numRight = 0, numWrong = 0;
    setNonCanonicalStdin();
    while (running && itemIndex < numItems) {
        clearScreen();
        updateHeader(currentWPM);
        std::string target = practiceItems[itemIndex];
        std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
        std::cout << "TARGET: " << target << "\n\n";
        std::cout << "(Press ESC to quit practice)\n";
        bool itemComplete = false;
        std::string typed;
        while (!itemComplete && running) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            FD_SET(STDIN_FILENO, &rfds);
            int maxfd = std::max(fd, STDIN_FILENO);
            struct timeval tv = {0, 100000};
            int ret = select(maxfd+1, &rfds, nullptr, nullptr, &tv);
            if (ret > 0) {
                if (FD_ISSET(STDIN_FILENO, &rfds)) {
                    char c;
                    if (read(STDIN_FILENO, &c, 1) > 0) {
                        if (c == 27) {
                            restoreStdin();
                            numWrong += (numItems - itemIndex);
                            clearScreen();
                            std::cout << "Practice aborted.\n\n";
                            goto showStats;
                        }
                    }
                }
                if (FD_ISSET(fd, &rfds)) {
                    unsigned char ch;
                    int n = read(fd, &ch, 1);
                    if (n > 0) {
                        if ((ch & 0xC0) == 0x80) {
                            int potVal = ch & 0x7F;
                            int newWPM = speedPotMinWpm + (potVal * (speedPotMaxWpm - speedPotMinWpm)) / 31;
                            if (newWPM != currentWPM) {
                                currentWPM = newWPM;
                                if (useBufferedSpeedChange)
                                    writeCmd(fd, 0x1C, static_cast<unsigned char>(currentWPM), true);
                                else
                                    writeCmd(fd, 0x02, static_cast<unsigned char>(currentWPM), true);
                                clearScreen();
                                updateHeader(currentWPM);
                                std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                std::cout << "TARGET: " << target << "\n\n";
                                std::cout << "You typed: " << typed << "\n\n";
                                std::cout << "(Press ESC to quit, Space/Enter to finalize)\n";
                            }
                        }
                        else if ((ch & 0xC0) == 0xC0) {
                        }
                        else {
                            if (ch >= 32 && ch <= 126) {
                                char mc = static_cast<char>(ch);
                                if (mc == ' ' || mc == '\r' || mc == '\n') {
                                    if (typed == target) {
                                        numRight++;
                                        itemComplete = true;
                                        clearScreen();
                                        updateHeader(currentWPM);
                                        std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                        std::cout << "TARGET: " << target << "\n\n";
                                        std::cout << "SUCCESS!\n\n";
                                        delay_ms(700);
                                    } else {
                                        numWrong++;
                                        itemComplete = true;
                                        clearScreen();
                                        updateHeader(currentWPM);
                                        std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                        std::cout << "TARGET: " << target << "\n\n";
                                        std::cout << "INCORRECT.\n\n";
                                        delay_ms(700);
                                    }
                                }
                                else if (mc == 8 || mc == 127) {
                                    if (!typed.empty())
                                        typed.pop_back();
                                    clearScreen();
                                    updateHeader(currentWPM);
                                    std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                    std::cout << "TARGET: " << target << "\n\n";
                                    std::cout << "You typed: " << typed << "\n\n";
                                    std::cout << "(Press ESC to quit, Space/Enter to finalize)\n";
                                }
                                else {
                                    typed.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(mc))));
                                    clearScreen();
                                    updateHeader(currentWPM);
                                    std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                    std::cout << "TARGET: " << target << "\n\n";
                                    std::cout << "You typed: " << typed << "\n\n";
                                    std::cout << "(Press ESC to quit, Space/Enter to finalize)\n";
                                }
                            }
                        }
                    }
                }
            }
        }
        itemIndex++;
    }
showStats:
    restoreStdin();
    {
        int total = numItems;
        clearScreen();
        std::cout << "Practice session finished!\n\n";
        std::cout << "Items asked: " << total << "\n";
        std::cout << "Right: " << numRight << "\n";
        std::cout << "Wrong: " << numWrong << "\n";
        double pct = (total > 0) ? 100.0 * static_cast<double>(numRight) / total : 0.0;
        std::cout << "Score: " << pct << "%\n\n";
        std::cout << "Press Enter to return to Main Menu...";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::string dummy;
        std::getline(std::cin, dummy);
    }
    restoreStdin();
}

void speedPracticeGameLoop(int fd) {
    clearScreen();
    std::cout << "===== SPEED PRACTICE MODE ======\n";
    std::cout << "This mode is like the Practice Game, but you set a countdown timer for each item.\n";
    std::cout << "Enter countdown time per item (seconds, from 5.0 to 0.1): ";
    std::string secLine;
    std::getline(std::cin, secLine);
    double timeLimitSec = 0.0;
    try { timeLimitSec = std::stod(secLine); } catch(...) { timeLimitSec = 0.0; }
    if (timeLimitSec > 5.0 || timeLimitSec < 0.1) {
        std::cout << "Invalid time limit. Press Enter to return...\n";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    clearScreen();
std::cout << "Select category:\n"
          << "1) Letters (A-Z)\n"
          << "2) Numbers (0-9)\n"
          << "3) Mix (letters + numbers)\n"
          << "4) Punctuation\n"
          << "0) Return\n"
          << "Enter option: ";

std::string line;
std::getline(std::cin, line);
if (line == "0") return;

std::vector<std::string> practiceItems;
if (line == "1") {
    // Letters
    for (char c = 'A'; c <= 'Z'; ++c)
        practiceItems.push_back(std::string(1, c));

} else if (line == "2") {
    // Numbers
    for (char c = '0'; c <= '9'; ++c)
        practiceItems.push_back(std::string(1, c));

} else if (line == "3") {
    // Mix letters + numbers
    for (char c = 'A'; c <= 'Z'; ++c)
        practiceItems.push_back(std::string(1, c));
    for (char c = '0'; c <= '9'; ++c)
        practiceItems.push_back(std::string(1, c));

} else if (line == "4") {
    // Punctuation
    practiceItems = {".", ",", "?", "/", "=", "-", ";"};

} else {
    std::cout << "Invalid category. Press Enter...\n";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return;
}

// everything below here remains the same...


//if (practiceItems.empty()) {
//    std::cout << "No items to practice. Press Enter...\n";
//    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
//    return;
//}

// etc.

    // REPLACE WITH:
std::random_device rd;
std::mt19937 rng(rd());
std::shuffle(practiceItems.begin(), practiceItems.end(), rng);

int numItems = static_cast<int>(practiceItems.size());
    std::vector<std::pair<std::string, std::string>> missed;
    int itemIndex = 0, numRight = 0, numWrong = 0;
    clearScreen();
    updateHeader(currentWPM);
    std::cout << "Speed Practice Setup Complete.\n\n"
              << "Time Limit per item: " << timeLimitSec << " seconds\n"
              << "Total items: " << numItems << "\n"
              << "Adjust WPM if needed.\n\n"
              << "Press SPACE (or Enter) to begin...\n"
              << "Press ESC to abort.\n";
    setNonCanonicalStdin();
    bool readyToStart = false;
    while (!readyToStart && running) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        int maxfd = std::max(fd, STDIN_FILENO);
        struct timeval tv = {0, 10000};
        int ret = select(maxfd+1, &rfds, nullptr, nullptr, &tv);
        if (ret > 0) {
            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                char c;
                if (read(STDIN_FILENO, &c, 1) > 0) {
                    if (c == 27) {
                        restoreStdin();
                        clearScreen();
                        std::cout << "Speed practice aborted.\n";
                        return;
                    }
                    else if (c == ' ' || c == '\n' || c == '\r')
                        readyToStart = true;
                }
            }
        }
    }
    if (!running) {
        restoreStdin();
        return;
    }
    while (running && itemIndex < numItems) {
        clearScreen();
        updateHeader(currentWPM);
        std::string target = practiceItems[itemIndex];
        auto startTime = std::chrono::steady_clock::now();
        bool itemComplete = false;
        std::string typed;
        while (!itemComplete && running) {
            double elapsed = std::chrono::duration<double>(
                std::chrono::steady_clock::now() - startTime).count();
            clearScreen();
            updateHeader(currentWPM);
            std::cout << "Speed Practice\n";
            std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
            std::cout << "TARGET: " << target << "\n\n";
            std::cout << "You typed: " << typed << "\n";
            std::cout << "(Press ESC to quit, Space/Enter to finalize)\n";
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);
            FD_SET(STDIN_FILENO, &rfds);
            int maxfd = std::max(fd, STDIN_FILENO);
            struct timeval tv = {0, 10000};
            int ret = select(maxfd+1, &rfds, nullptr, nullptr, &tv);
            if (ret > 0) {
                if (FD_ISSET(STDIN_FILENO, &rfds)) {
                    char c;
                    if (read(STDIN_FILENO, &c, 1) > 0) {
                        if (c == 27) {
                            restoreStdin();
                            numWrong += (numItems - itemIndex);
                            clearScreen();
                            std::cout << "Speed practice aborted.\n\n";
                            goto showStatsSpeed;
                        }
                    }
                }
                if (FD_ISSET(fd, &rfds)) {
                    unsigned char ch;
                    int n = read(fd, &ch, 1);
                    if (n > 0) {
                        if ((ch & 0xC0) == 0x80) {
                            int potVal = ch & 0x7F;
                            int newWPM = speedPotMinWpm + (potVal * (speedPotMaxWpm - speedPotMinWpm)) / 31;
                            if (newWPM != currentWPM) {
                                currentWPM = newWPM;
                                if (useBufferedSpeedChange)
                                    writeCmd(fd, 0x1C, static_cast<unsigned char>(currentWPM), true);
                                else
                                    writeCmd(fd, 0x02, static_cast<unsigned char>(currentWPM), true);
                            }
                        }
                        else if ((ch & 0xC0) == 0xC0) {
                        }
                        else {
                            if (ch >= 32 && ch <= 126) {
                                char mc = static_cast<char>(ch);
                                if (mc == ' ' || mc == '\r' || mc == '\n') {
                                    std::string finalTyped = typed;
                                    finalTyped.erase(finalTyped.find_last_not_of(" \n\r\t")+1);
                                    if (finalTyped == target) {
                                        numRight++;
                                        clearScreen();
                                        updateHeader(currentWPM);
                                        std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                        std::cout << "TARGET: " << target << "\n\n";
                                        std::cout << "SUCCESS!\n\n";
                                       // delay_ms(700);
                                    } else {
                                        numWrong++;
                                        clearScreen();
                                        updateHeader(currentWPM);
                                        std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                                        std::cout << "TARGET: " << target << "\n\n";
                                        std::cout << "INCORRECT.\n\n";
                                       // delay_ms(700);
                                        missed.push_back({target, finalTyped});
                                    }
                                    itemComplete = true;
                                }
                                else if (mc == 8 || mc == 127) {
                                    if (!typed.empty())
                                        typed.pop_back();
                                }
                                else {
                                    typed.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(mc))));
                                }
                            }
                        }
                    }
                }
            }
            if (elapsed >= timeLimitSec) {
                numWrong++;
                std::string finalTyped = typed;
                finalTyped.erase(finalTyped.find_last_not_of(" \n\r\t")+1);
                missed.push_back({target, finalTyped});
                clearScreen();
                updateHeader(currentWPM);
                std::cout << "Item " << (itemIndex+1) << " of " << numItems << "\n";
                std::cout << "TARGET: " << target << "\n\n";
                std::cout << "TIME EXPIRED. INCORRECT.\n\n";
                delay_ms(700);
                itemComplete = true;
            }
        }
        itemIndex++;
    }
showStatsSpeed:
    restoreStdin();
    {
        int total = numItems;
        clearScreen();
        std::cout << "Speed Practice session finished!\n\n";
        std::cout << "Items asked: " << total << "\n";
        std::cout << "Right: " << numRight << "\n";
        std::cout << "Wrong: " << numWrong << "\n";
        double pct = (total > 0) ? 100.0 * static_cast<double>(numRight) / total : 0.0;
        std::cout << "Score: " << pct << "%\n\n";
        if (!missed.empty()) {
            std::cout << "Missed Questions:\n";
            for (auto &p : missed) {
                std::cout << "TARGET: " << p.first << " | You answered: ";
                if (p.second.empty())
                    std::cout << "(no answer)";
                else
                    std::cout << p.second;
                std::cout << "\n";
            }
            std::cout << "\n";
        }
        std::cout << "Press Enter to return to Main Menu...";
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::string dummy;
        std::getline(std::cin, dummy);
    }
}

void showPinConfigSubMenu(int fd) {
    while (true) {
        clearScreen();
        std::cout << "====== PIN CONFIGURATION MENU ======\n"
                  << "1) Internal speaker ON (0x0F)\n"
                  << "2) Internal speaker OFF (0x0D)\n"
                  << "3) Some custom config (0x07)\n"
                  << "0) Return to Previous Menu\n\n"
                  << "Enter choice: ";
        std::string line;
        std::getline(std::cin, line);
        if (line == "0")
            break;
        else if (line == "1") {
            pinCfg = 0x0F;
            internalSpeakerOn = true;
        } else if (line == "2") {
            pinCfg = 0x0D;
            internalSpeakerOn = false;
        } else if (line == "3") {
            pinCfg = 0x07;
        } else {
            std::cout << "\nInvalid choice.\nPress Enter to continue...";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        writeCmd(fd, 0x09, pinCfg, true);
        std::cout << "\nPin config set to 0x" << std::hex << (int)pinCfg << std::dec << ".\n";
        std::cout << "Press Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void showSettingsMenu(int fd) {
    while(true) {
        clearScreen();
        std::cout << "========== SETTINGS MENU ==========\n";
        std::cout << "1) Set Speed (WPM)\n";
        std::cout << "2) Set Weighting (percent)\n";
        std::cout << "3) Set Farnsworth Speed (WPM)\n";
        std::cout << "4) Set Dit/Dah Ratio (33-66)\n";
        std::cout << "5) Set Key Compensation (ms)\n";
        std::cout << "0) Return\n";
        std::cout << "Enter option: ";
        std::string line;
        std::getline(std::cin, line);
        if (line == "0") break;
        int opt = -1;
        try { opt = std::stoi(line); } catch(...) { std::cout << "Invalid.\n"; continue; }
        clearScreen();
        switch(opt) {
        case 1: {
            std::cout << "Enter Speed (WPM, 5-99): ";
            std::getline(std::cin, line);
            int spd = 0;
            try { spd = std::stoi(line); } catch(...) { spd = 0; }
            if (spd < 5 || spd > 99)
                std::cout << "Invalid speed.\n";
            else {
                if (useBufferedSpeedChange)
                    writeCmd(fd, 0x1C, static_cast<unsigned char>(spd), true);
                else
                    writeCmd(fd, 0x02, static_cast<unsigned char>(spd), true);
                currentWPM = spd;
                updateHeader(currentWPM);
                std::cout << "Speed set to " << spd << " WPM.\n";
            }
            std::cout << "Press Enter to continue...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 2: {
            std::cout << "Enter Weighting (10-90): ";
            std::getline(std::cin, line);
            int w = 0;
            try { w = std::stoi(line); } catch(...) { w = 0; }
            if (w < 10 || w > 90)
                std::cout << "Invalid weighting.\n";
            else {
                writeCmd(fd, 0x03, static_cast<unsigned char>(w), true);
                std::cout << "Weighting set to " << w << "%.\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 3: {
            std::cout << "Enter Farnsworth Speed (10-99): ";
            std::getline(std::cin, line);
            int f = 0;
            try { f = std::stoi(line); } catch(...) { f = 0; }
            if (f < 10 || f > 99)
                std::cout << "Invalid Farnsworth.\n";
            else {
                writeCmd(fd, 0x0D, static_cast<unsigned char>(f), true);
                std::cout << "Farnsworth speed: " << f << " WPM.\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 4: {
            std::cout << "Enter Dit/Dah Ratio (33-66): ";
            std::getline(std::cin, line);
            int ratio = 0;
            try { ratio = std::stoi(line); } catch(...) { ratio = 0; }
            if (ratio < 33 || ratio > 66)
                std::cout << "Invalid ratio.\n";
            else {
                writeCmd(fd, 0x17, static_cast<unsigned char>(ratio), true);
                std::cout << "Dit/Dah ratio: " << ratio << ".\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 5: {
            std::cout << "Enter Key Compensation (0-250 ms): ";
            std::getline(std::cin, line);
            int comp = 0;
            try { comp = std::stoi(line); } catch(...) { comp = -1; }
            if (comp < 0 || comp > 250)
                std::cout << "Invalid.\n";
            else {
                writeCmd(fd, 0x11, static_cast<unsigned char>(comp), true);
                std::cout << "Key Compensation: " << comp << " ms.\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        default:
            std::cout << "Invalid.\n";
            break;
        }
    }
}

void showHardwareOptionsMenu(int fd) {
    while (true) {
        clearScreen();
        std::cout << "====== HARDWARE OPTIONS MENU ======\n";
        std::cout << "1) Set PTT Lead-In Delay (ms)\n";
        std::cout << "2) Set PTT Tail Delay (ms)\n";
        std::cout << "3) Set Pin Configuration\n";
        std::cout << "4) Set Speed Pot Range\n";
        std::cout << "5) Toggle Internal Speaker (" << (internalSpeakerOn ? "ON" : "OFF") << ")\n";
        std::cout << "0) Return\n";
        std::cout << "Enter option: ";
        std::string line;
        std::getline(std::cin, line);
        if (line == "0") break;
        int opt = -1;
        try { opt = std::stoi(line); } catch(...) { std::cout << "Invalid.\n"; continue; }
        clearScreen();
        switch (opt) {
        case 1: {
            std::cout << "Enter PTT Lead-In Delay (0-250 ms): ";
            std::getline(std::cin, line);
            int lead = 0;
            try { lead = std::stoi(line); } catch(...) { lead = -1; }
            if (lead < 0 || lead > 250)
                std::cout << "Invalid.\n";
            else {
                int leadByte = lead / 10;
                unsigned char cmd[5];
                cmd[0] = 0x04;
                cmd[1] = static_cast<unsigned char>(leadByte);
                cmd[2] = 0;
                cmd[3] = 0;
                cmd[4] = 0;
                writeBytes(fd, cmd, 5);
                std::cout << "PTT Lead-In: " << lead << " ms.\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 2: {
            std::cout << "Enter PTT Tail Delay (0-250 ms): ";
            std::getline(std::cin, line);
            int tail = 0;
            try { tail = std::stoi(line); } catch(...) { tail = -1; }
            if (tail < 0 || tail > 250)
                std::cout << "Invalid.\n";
            else {
                int tailByte = tail / 10;
                unsigned char cmd[5];
                cmd[0] = 0x04;
                cmd[1] = 0;
                cmd[2] = static_cast<unsigned char>(tailByte);
                cmd[3] = 0;
                cmd[4] = 0;
                writeBytes(fd, cmd, 5);
                std::cout << "PTT Tail Delay: " << tail << " ms.\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 3: {
            showPinConfigSubMenu(fd);
            break;
        }
        case 4: {
            std::cout << "Enter Speed Pot MIN WPM: ";
            std::getline(std::cin, line);
            int minW = 0;
            try { minW = std::stoi(line); } catch(...) { minW = -1; }
            std::cout << "Enter Speed Pot MAX WPM: ";
            std::string line2;
            std::getline(std::cin, line2);
            int maxW = 0;
            try { maxW = std::stoi(line2); } catch(...) { maxW = -1; }
            if (maxW <= minW || minW < 1)
                std::cout << "Invalid.\n";
            else {
                int range = maxW - minW;
                unsigned char cmd[4] = {0x05, static_cast<unsigned char>(minW), static_cast<unsigned char>(range), 0};
                writeBytes(fd, cmd, 4);
                speedPotMinWpm = minW;
                speedPotMaxWpm = maxW;
                std::cout << "Speed Pot Range: " << minW << " - " << maxW << "\n";
            }
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        case 5: {
            internalSpeakerOn = !internalSpeakerOn;
            pinCfg = (internalSpeakerOn ? 0x0F : 0x0D);
            writeCmd(fd, 0x09, pinCfg, true);
            std::cout << "Speaker => " << (internalSpeakerOn ? "ON" : "OFF") << "\n";
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        default:
            std::cout << "Invalid.\n";
            break;
        }
    }
}

void showOtherOptionsMenu(int fd) {
    while (true) {
        clearScreen();
        std::cout << "======= OTHER OPTIONS MENU =======\n";
        std::cout << "1) Set WinKeyer Mode\n";
        std::cout << "2) Toggle Speed Command Mode (currently " 
                  << (useBufferedSpeedChange ? "buffered (0x1C)" : "direct (0x02)") << ")\n";
        std::cout << "0) Return\n";
        std::cout << "Enter option: ";
        std::string line;
        std::getline(std::cin, line);
        if (line == "0") break;
        int opt = -1;
        try { opt = std::stoi(line); } catch(...) { std::cout << "Invalid.\n"; continue; }
        clearScreen();
        switch (opt) {
        case 1: {
            while (true) {
                clearScreen();
                std::cout << "--- SET WINKEYER MODE ---\n";
                std::cout << "1) Iambic B (default)\n"
                          << "2) Iambic A\n"
                          << "3) Ultimatic\n"
                          << "4) Bug Mode\n"
                          << "0) Return\n"
                          << "Choice? ";
                std::getline(std::cin, line);
                if (line == "0") break;
                int mOpt = -1;
                try { mOpt = std::stoi(line); } catch(...) { std::cout << "Invalid.\n"; continue; }
                unsigned char modeVal = 0;
                switch(mOpt) {
                    case 1: modeVal = 0xC4; break;
                    case 2: modeVal = 0xD4; break;
                    case 3: modeVal = 0xE4; break;
                    case 4: modeVal = 0xF4; break;
                    default:
                        std::cout << "Invalid.\n";
                        continue;
                }
                writeCmd(fd, 0x0E, modeVal, true);
                std::cout << "Mode set.\nPress Enter...\n";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            break;
        }
        case 2: {
            useBufferedSpeedChange = !useBufferedSpeedChange;
            std::cout << "Now using " 
                      << (useBufferedSpeedChange ? "buffered (0x1C)" : "direct (0x02)") 
                      << " speed.\n";
            std::cout << "Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            break;
        }
        default:
            std::cout << "Invalid.\n";
            break;
        }
    }
}

std::string getMainMenuOption() {
    clearScreen();
    std::cout << "========== Winkeyer Menu ==========\n"
              << "1) Settings\n"
              << "2) Hardware Options\n"
              << "3) Other Options\n"
              << "4) Practice Game\n"
              << "5) Speed Practice\n"
              << "0) Back\n"
              << "Enter option: ";
    std::string opt;
    std::getline(std::cin, opt);
    return opt;
}

void winkeyerMain() {
    signal(SIGINT, sigint_handler);
    std::string device = "/dev/ttyUSB0";
    int fd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Error opening " << device << ": " << strerror(errno) << "\n";
        return;
    }
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "tcgetattr error: " << strerror(errno) << "\n";
        close(fd);
        return;
    }
    cfmakeraw(&tty);
    cfsetispeed(&tty, B1200);
    cfsetospeed(&tty, B1200);
    tty.c_cflag |= CSTOPB;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "tcsetattr() error: " << strerror(errno) << "\n";
        close(fd);
        return;
    }
    if (!initWinkeyer(fd)) {
        close(fd);
        return;
    }
    bool exitProgram = false;
    while (!exitProgram && running) {
        std::string choice = getMainMenuOption();
        if (choice == "1")
            showSettingsMenu(fd);
        else if (choice == "2")
            showHardwareOptionsMenu(fd);
        else if (choice == "3")
            showOtherOptionsMenu(fd);
        else if (choice == "4")
            practiceGameLoop(fd);
        else if (choice == "5")
            speedPracticeGameLoop(fd);
        else if (choice == "0")
            exitProgram = true;
        else {
            std::cout << "Invalid option. Press Enter...\n";
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }
    close(fd);
    std::cout << "\nExiting WinKeyer module...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

} // end namespace WinKeyerModule

// ------------------------------------------------------------
// Top-Level Main Menu
// ------------------------------------------------------------
int main() {
    while (true) {
        globalClearScreen();
        std::cout << "=====Morse Code========\n"
                  << "1: Patrice Receiving\n"
                  << "2: Patrice Sending\n"
                  << "3: Exit Program\n"
                  << "Enter your choise (1-3) ";
        int choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if (choice == 1) {
            MorseModule::morseMain();
        } else if (choice == 2) {
            WinKeyerModule::winkeyerMain();
        } else if (choice == 3) {
            break;
        } else {
            std::cout << "Invalid option. Press Enter to try again.";
            std::cin.get();
        }
    }
    return 0;
}
