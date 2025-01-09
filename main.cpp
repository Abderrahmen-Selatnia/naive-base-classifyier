#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <map>
#include <cmath>
#include <algorithm>
#include <iomanip>

const int SCREEN_WIDTH = 1900;
const int SCREEN_HEIGHT = 900;
const float RECT_SCALE = 0.3;
const int EMAIL_WIDTH = 70 * RECT_SCALE;
const int EMAIL_HEIGHT = 35 * RECT_SCALE;
const int GRID_SPACING = 40;
const int SCROLL_SPEED = 10;

int gridOffsetX = 0;
bool isDragging = false;
int lastMouseX = 0;

SDL_Color RED = {255, 0, 0, 255};
SDL_Color GREEN = {0, 255, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};
SDL_Color GRAY = {169, 169, 169, 255};
SDL_Color YELLOW = {255, 255, 0, 255};

struct WordStats {
    std::string word;
    int spamCount;
    int nonSpamCount;
    double spamProbability;
    
    WordStats() : spamCount(0), nonSpamCount(0), spamProbability(0.0) {}
};

struct Email {
    int x, y;
    int target_x, target_y;
    SDL_Color color;
    bool blinking;
    std::string address;
    std::string content;
    bool is_spam;
    float current_x, current_y;

    Email(int x, int y, std::string address, std::string content)
        : x(x), y(y), current_x(x), current_y(y), target_x(x), target_y(y),
          address(address), content(content), color(WHITE), blinking(false), is_spam(false) {}

    void updatePosition() {
        const float LERP_FACTOR = 0.1f;
        current_x += (target_x - current_x) * LERP_FACTOR;
        current_y += (target_y - current_y) * LERP_FACTOR;
    }
};
class NaiveBayesClassifier {
private:
    std::unordered_map<std::string, double> spamWordProbs;
    std::unordered_map<std::string, double> nonSpamWordProbs;
    double spamPrior;
    double nonSpamPrior;

public:
    void train(const std::vector<Email>& trainingSet) {
        int spamCount = 0;
        int totalEmails = trainingSet.size();
        
        std::unordered_map<std::string, int> spamWordCounts;
        std::unordered_map<std::string, int> nonSpamWordCounts;
        std::unordered_map<std::string, int> vocabulary;
        
        for (const Email& email : trainingSet) {
            if (email.is_spam) spamCount++;
            std::istringstream ss(email.content);
            std::string word;
            while (ss >> word) {
                std::transform(word.begin(), word.end(), word.begin(), ::tolower);
                vocabulary[word]++;
                if (email.is_spam) {
                    spamWordCounts[word]++;
                } else {
                    nonSpamWordCounts[word]++;
                }
            }
        }
        
        spamPrior = static_cast<double>(spamCount) / totalEmails;
        nonSpamPrior = 1.0 - spamPrior;
        
        int vocabSize = vocabulary.size();
        for (const auto& word : vocabulary) {
            spamWordProbs[word.first] = (spamWordCounts[word.first] + 1.0) / (spamCount + vocabSize);
            nonSpamWordProbs[word.first] = (nonSpamWordCounts[word.first] + 1.0) / (totalEmails - spamCount + vocabSize);
        }
    }
    
    bool classify(const std::string& content) {
        double spamScore = log(spamPrior);
        double nonSpamScore = log(nonSpamPrior);
        
        std::istringstream ss(content);
        std::string word;
        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (spamWordProbs.count(word)) {
                spamScore += log(spamWordProbs[word]);
                nonSpamScore += log(nonSpamWordProbs[word]);
            }
        }
        
        return spamScore > nonSpamScore;
    }

    const std::unordered_map<std::string, double>& getSpamWordProbs() const {
        return spamWordProbs;
    }
    
    const std::unordered_map<std::string, double>& getNonSpamWordProbs() const {
        return nonSpamWordProbs;
    }
};


void handleMouseEvents(SDL_Event& event, bool& isDragging, int& gridOffsetX, int& lastMouseX) {
    switch (event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                isDragging = true;
                lastMouseX = event.button.x;
            }
            break;
            
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                isDragging = false;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (isDragging) {
                int deltaX = event.motion.x - lastMouseX;
                gridOffsetX += deltaX;
                lastMouseX = event.motion.x;
            }
            break;
    }
}

std::unordered_map<std::string, bool> loadWordList(const std::string& filename) {
    std::unordered_map<std::string, bool> wordList;
    std::ifstream file(filename);
    std::string word;
    
    while (std::getline(file, word)) {
        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        wordList[word] = true;
    }
    return wordList;
}

void generateDetailedReports(const std::vector<Email>& emails, 
                           const std::unordered_map<std::string, double>& spamWordProbs,
                           const std::unordered_map<std::string, double>& nonSpamWordProbs) {
    std::map<std::string, WordStats> wordStats;
    
    for (const Email& email : emails) {
        std::istringstream ss(email.content);
        std::string word;
        while (ss >> word) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            WordStats& stats = wordStats[word];
            stats.word = word;
            if (email.is_spam) {
                stats.spamCount++;
            } else {
                stats.nonSpamCount++;
            }
            stats.spamProbability = spamWordProbs.count(word) ? spamWordProbs.at(word) : 0.0;
        }
    }

    std::ofstream reportFile("classification_report.txt");
    reportFile << "Word Classification Report\n";
    reportFile << "========================\n\n";
    
    for (const auto& pair : wordStats) {
        reportFile << "Word: " << pair.second.word << "\n";
        reportFile << "Spam Count: " << pair.second.spamCount << "\n";
        reportFile << "Non-Spam Count: " << pair.second.nonSpamCount << "\n";
        reportFile << "Spam Probability: " << pair.second.spamProbability << "\n\n";
    }
}
std::vector<Email> loadEmails(const std::string &filename) {
    std::vector<Email> emails;
    std::ifstream file(filename);
    std::string line;

    if (file.is_open()) {
        const int START_X = 50;
        const int START_Y = 50;
        const int EMAILS_PER_COLUMN = (SCREEN_HEIGHT - 100) / (EMAIL_HEIGHT + GRID_SPACING);
        
        int currentX = START_X;
        int currentY = START_Y;
        int count = 0;
        int maxColumnWidth = EMAIL_WIDTH;
        std::vector<Email> currentColumn;
        
        while (std::getline(file, line)) {
            std::istringstream ss(line);
            std::string address, content;

            std::getline(ss, address, ',');
            std::getline(ss, content);

            if (!address.empty() && !content.empty()) {
                Email email(currentX, currentY, address, content);
                currentColumn.push_back(email);
                
                int addressWidth = address.length() * 8;
                maxColumnWidth = std::max(maxColumnWidth, EMAIL_WIDTH + addressWidth);
                
                count++;
                currentY += EMAIL_HEIGHT + GRID_SPACING;
                
                if (count % EMAILS_PER_COLUMN == 0) {
                    emails.insert(emails.end(), currentColumn.begin(), currentColumn.end());
                    currentColumn.clear();
                    currentY = START_Y;
                    currentX += maxColumnWidth + GRID_SPACING;
                    maxColumnWidth = EMAIL_WIDTH;
                }
            }
        }
        
        if (!currentColumn.empty()) {
            emails.insert(emails.end(), currentColumn.begin(), currentColumn.end());
        }
        
        file.close();
    }
    return emails;
}

void renderEmailRect(SDL_Renderer* renderer, const Email& email) {
    SDL_Rect emailRect = {
        static_cast<int>(email.current_x) + gridOffsetX, 
        static_cast<int>(email.current_y), 
        EMAIL_WIDTH, 
        EMAIL_HEIGHT
    };
    SDL_SetRenderDrawColor(renderer, email.color.r, email.color.g, email.color.b, 255);
    SDL_RenderFillRect(renderer, &emailRect);
}

void renderClassifier(SDL_Renderer* renderer, int x, int y, const std::string& currentWord, 
                     const std::unordered_map<std::string, bool>& spamWords,
                     const std::unordered_map<std::string, bool>& nonSpamWords) {
    // Draw multiple rectangles for bold effect
    for(int i = 0; i < 3; i++) {  // Increased thickness
        SDL_Rect classifierRect = {
            x - 5 - i + gridOffsetX, 
            y - 5 - i, 
            EMAIL_WIDTH + 10 + (i*2), 
            EMAIL_HEIGHT + 10 + (i*2)
        };
        
        std::string wordLower = currentWord;
        std::transform(wordLower.begin(), wordLower.end(), wordLower.begin(), ::tolower);
        
        if (spamWords.count(wordLower)) {
            SDL_SetRenderDrawColor(renderer, RED.r, RED.g, RED.b, 255);
        } else if (nonSpamWords.count(wordLower)) {
            SDL_SetRenderDrawColor(renderer, GREEN.r, GREEN.g, GREEN.b, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, 255);
        }
        
        SDL_RenderDrawRect(renderer, &classifierRect);
    }
}

void renderEmailAddress(SDL_Renderer *renderer, TTF_Font *font, const Email &email) {
    SDL_Surface *surface = TTF_RenderText_Solid(font, email.address.c_str(), WHITE);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {
        static_cast<int>(email.current_x + EMAIL_WIDTH + 15) + gridOffsetX, // Increased from 5 to 15
        static_cast<int>(email.current_y),
        surface->w,
        surface->h
    };
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

// [Previous generateDetailedReports implementation remains the same]

void classifyEmails(SDL_Renderer *renderer, TTF_Font *font, std::vector<Email> &emails) {
    NaiveBayesClassifier classifier;
    auto spamWords = loadWordList("spam_words.txt");
    auto nonSpamWords = loadWordList("non_spam_words.txt");
    
    
    // Initial training phase
    for (Email& email : emails) {
        std::istringstream ss(email.content);
        std::string word;
        bool hasSpamWord = false;
        while (ss >> word && !hasSpamWord) {
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (spamWords.count(word)) {
                hasSpamWord = true;
            }
        }
        email.is_spam = hasSpamWord;
    }
    
    classifier.train(emails);
    
    // Classification phase with visualization
    for (Email& email : emails) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        SDL_Event event;
        std::istringstream ss(email.content);
        std::string word;
       while (ss >> word) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q) {
                return; // Return to main where cleanup will happen
            }
            handleMouseEvents(event, isDragging, gridOffsetX, lastMouseX);
        }
            
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            
            for (const Email& e : emails) {
                renderEmailRect(renderer, e);
                renderEmailAddress(renderer, font, e);
            }
            
            renderClassifier(renderer, email.current_x, email.current_y, word, spamWords, nonSpamWords);
            SDL_RenderPresent(renderer);
            SDL_Delay(30);
        }
        
        email.is_spam = classifier.classify(email.content);
        email.color = email.is_spam ? RED : GREEN;
    }

    const int SPAM_START_X = 50;
    const int NON_SPAM_START_X = SCREEN_WIDTH - 250;
    int spam_y = 50;
    int non_spam_y = 50;

    for (Email& email : emails) {
        if (email.is_spam) {
            email.target_x = SPAM_START_X;
            email.target_y = spam_y;
            spam_y += EMAIL_HEIGHT + GRID_SPACING;
        } else {
            email.target_x = NON_SPAM_START_X;
            email.target_y = non_spam_y;
            non_spam_y += EMAIL_HEIGHT + GRID_SPACING;
        }
    }

    bool moving = true;
    while (moving) {
        moving = false;
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (Email& email : emails) {
            email.updatePosition();
            if (std::abs(email.current_x - email.target_x) > 0.1 || 
                std::abs(email.current_y - email.target_y) > 0.1) {
                moving = true;
            }
            renderEmailRect(renderer, email);
            renderEmailAddress(renderer, font, email);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    generateDetailedReports(emails, classifier.getSpamWordProbs(), classifier.getNonSpamWordProbs());
}

int main(int argc, char *argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1) {
        std::cerr << "Initialization failed: " << SDL_GetError() << " " << TTF_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Email Classifier - Naive Bayes Visualization",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    TTF_Font *font = TTF_OpenFont("Arial.ttf", 14);

    if (!window || !renderer || !font) {
        std::cerr << "Setup failed: " << SDL_GetError() << " " << TTF_GetError() << std::endl;
        return 1;
    }

    std::vector<Email> emails = loadEmails("emails.txt");
    if (emails.empty()) {
        std::cerr << "No emails loaded from file" << std::endl;
        return 1;
    }

    classifyEmails(renderer, font, emails);

    bool running = true;
    SDL_Event event;
   while (running) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || 
            (event.type == SDL_KEYDOWN && 
             (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q))) {
            running = false;
        }
        handleMouseEvents(event, isDragging, gridOffsetX, lastMouseX);
    }
        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        for (const Email& email : emails) {
            renderEmailRect(renderer, email);
            renderEmailAddress(renderer, font, email);
        }
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
