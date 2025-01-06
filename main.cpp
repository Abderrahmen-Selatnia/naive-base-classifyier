#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

const int SCREEN_WIDTH = 1900;
const int SCREEN_HEIGHT = 900;
const float RECT_SCALE = 0.3;
const int EMAIL_WIDTH = 70 * RECT_SCALE;
const int EMAIL_HEIGHT = 35 * RECT_SCALE;

SDL_Color RED = {255, 0, 0, 255};
SDL_Color GREEN = {0, 255, 0, 255};
SDL_Color WHITE = {255, 255, 255, 255};
SDL_Color GRAY = {169, 169, 169, 255};

struct Email
{
    int x, y;
    int target_x, target_y;
    SDL_Color color;
    bool blinking;
    std::string address;
    std::string content;
    bool is_spam;

    Email(int x, int y, std::string address, std::string content)
        : x(x), y(y), address(address), content(content), color(WHITE), blinking(false), is_spam(false) {}

    void toggleBlinking()
    {
        blinking = !blinking;
    }

    void classify()
    {
        if (content.find("free") != std::string::npos || content.find("offer") != std::string::npos)
        {
            color = RED;
            is_spam = true;
        }
        else
        {
            color = GREEN;
            is_spam = false;
        }
    }

    void move(int speed = 3)
    {
        if (x < target_x)
            x += speed;
        if (x > target_x)
            x -= speed;
        if (y < target_y)
            y += speed;
        if (y > target_y)
            y -= speed;
    }
};

std::vector<Email> loadEmails(const std::string &filename)
{
    std::vector<Email> emails;
    std::ifstream file(filename);
    std::string line;

    if (file.is_open())
    {
        int yPos = 100;
        while (std::getline(file, line))
        {
            std::istringstream ss(line);
            std::string address, content;

            std::getline(ss, address, ',');
            std::getline(ss, content);

            if (!address.empty() && !content.empty())
            {
                emails.push_back(Email(100, yPos, address, content));
                yPos += EMAIL_HEIGHT + 20;
            }
        }
        file.close();
    }
    else
    {
        std::cerr << "Failed to open file: " << filename << std::endl;
    }

    return emails;
}

void renderEmailAddress(SDL_Renderer *renderer, TTF_Font *font, const Email &email, int x, int y)
{
    SDL_Surface *surface = TTF_RenderText_Solid(font, email.address.c_str(), WHITE);
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect destRect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(renderer, texture, NULL, &destRect);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void renderClassifiedEmails(SDL_Renderer *renderer, TTF_Font *font, const std::vector<Email> &spamEmails, const std::vector<Email> &nonSpamEmails)
{
    const int COLUMN_WIDTH = 100;
    const int COLUMN_SPACING = 20;
    const int ROW_SPACING = 20;

    int email_x = 50, email_y = 50;
    for (const Email &email : spamEmails)
    {
        SDL_Rect emailRect = {email_x, email_y, EMAIL_WIDTH, EMAIL_HEIGHT};
        SDL_SetRenderDrawColor(renderer, RED.r, RED.g, RED.b, 255);
        SDL_RenderFillRect(renderer, &emailRect);
        renderEmailAddress(renderer, font, email, email_x + EMAIL_WIDTH + 5, email_y);
        email_y += EMAIL_HEIGHT + ROW_SPACING;

        if (email_y >= SCREEN_HEIGHT - 50)
        {
            email_y = 50;
            email_x += COLUMN_WIDTH + COLUMN_SPACING;
        }
    }

    email_x = SCREEN_WIDTH - 50;
    email_y = 50;
    for (const Email &email : nonSpamEmails)
    {
        SDL_Rect emailRect = {email_x, email_y, EMAIL_WIDTH, EMAIL_HEIGHT};
        SDL_SetRenderDrawColor(renderer, GREEN.r, GREEN.g, GREEN.b, 255);
        SDL_RenderFillRect(renderer, &emailRect);

        int emailWidth = email.address.length() * 7;
        renderEmailAddress(renderer, font, email, email_x - emailWidth - 2, email_y);
        email_y += EMAIL_HEIGHT + ROW_SPACING;

        if (email_y >= SCREEN_HEIGHT - 50)
        {
            email_y = 50;
            email_x -= COLUMN_WIDTH + COLUMN_SPACING;
        }
    }

    SDL_RenderPresent(renderer);
}

void classifyEmails(SDL_Renderer *renderer, TTF_Font *font, std::vector<Email> &emails)
{
    const int MAX_COLS = 8;
    const int COLUMN_WIDTH = 200;
    const int COLUMN_SPACING = 20;
    const int ROW_SPACING = 20;

    int gridStartX = (SCREEN_WIDTH - (MAX_COLS * (COLUMN_WIDTH + COLUMN_SPACING) - COLUMN_SPACING)) / 2;
    int gridStartY = 50;

    int num_rows = (emails.size() + MAX_COLS - 1) / MAX_COLS;

    int email_x = gridStartX, email_y = gridStartY;
    for (int i = 0; i < emails.size(); i++)
    {
        emails[i].x = email_x;
        emails[i].y = email_y;

        email_y += EMAIL_HEIGHT + ROW_SPACING;
        if ((i + 1) % num_rows == 0)
        {
            email_x += COLUMN_WIDTH + COLUMN_SPACING;
            email_y = gridStartY;
        }
    }

    SDL_Rect classifierShape = {50, 50, 20, 20};

    int currentIndex = 0;

    TTF_Font *largeFont = TTF_OpenFont("Arial.ttf", 24); // Larger font for the current address text

    for (int col = 0; col < MAX_COLS; col++)
    {
        for (int row = 0; row < num_rows; row++)
        {
            if (currentIndex >= emails.size())
                break;

            Email &email = emails[currentIndex];

            while (classifierShape.x != email.x || classifierShape.y != email.y)
            {
                if (classifierShape.y < email.y)
                    classifierShape.y++;
                if (classifierShape.y > email.y)
                    classifierShape.y--;
                if (classifierShape.x < email.x)
                    classifierShape.x++;
                if (classifierShape.x > email.x)
                    classifierShape.x--;

                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);

                for (const Email &e : emails)
                {
                    SDL_Rect emailRect = {e.x, e.y, EMAIL_WIDTH, EMAIL_HEIGHT};
                    SDL_SetRenderDrawColor(renderer, e.color.r, e.color.g, e.color.b, 255);
                    SDL_RenderFillRect(renderer, &emailRect);
                }

                SDL_SetRenderDrawColor(renderer, GRAY.r, GRAY.g, GRAY.b, 255);
                SDL_RenderFillRect(renderer, &classifierShape);

                // Render current address at the bottom
                SDL_Surface *addressSurface = TTF_RenderText_Solid(largeFont, email.address.c_str(), WHITE);
                SDL_Texture *addressTexture = SDL_CreateTextureFromSurface(renderer, addressSurface);
                SDL_Rect addressRect = {50, SCREEN_HEIGHT - 50 - addressSurface->h, addressSurface->w, addressSurface->h};
                SDL_RenderCopy(renderer, addressTexture, NULL, &addressRect);
                SDL_DestroyTexture(addressTexture);
                SDL_FreeSurface(addressSurface);

                SDL_RenderPresent(renderer);
                SDL_Delay(15);//classifier delay
            }

            email.toggleBlinking();
            SDL_Rect emailRect = {email.x, email.y, EMAIL_WIDTH, EMAIL_HEIGHT};

            bool containsSpamWord = email.content.find("free") != std::string::npos || email.content.find("offer") != std::string::npos;

            SDL_Color classifierColor = containsSpamWord ? RED : GREEN;

            for (int i = 0; i < 4; i++) {
                SDL_SetRenderDrawColor(renderer, email.blinking ? classifierColor.r : GRAY.r,
                                    email.blinking ? classifierColor.g : GRAY.g,
                                    email.blinking ? classifierColor.b : GRAY.b, 255);

                SDL_RenderFillRect(renderer, &classifierShape);
                SDL_RenderFillRect(renderer, &emailRect);
                SDL_RenderPresent(renderer);

                SDL_Delay(100);

                email.toggleBlinking();
            }

            email.classify();
            currentIndex++;
        }
    }

    std::vector<Email> spamEmails;
    std::vector<Email> nonSpamEmails;

    for (Email &email : emails)
    {
        if (email.is_spam)
        {
            spamEmails.push_back(email);
        }
        else
        {
            nonSpamEmails.push_back(email);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    renderClassifiedEmails(renderer, font, spamEmails, nonSpamEmails);

    TTF_CloseFont(largeFont); // Close the large font
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0 || TTF_Init() == -1)
    {
        std::cerr << "Initialization failed: " << SDL_GetError() << " " << TTF_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Email Classifier", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font *font = TTF_OpenFont("Arial.ttf", 12);

    if (!window || !renderer || !font)
    {
        std::cerr << "Setup failed: " << SDL_GetError() << " " << TTF_GetError() << std::endl;
        return 1;
    }

    std::vector<Email> emails = loadEmails("emails.txt");

    classifyEmails(renderer, font, emails);

    SDL_Event e;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                quit = true;
        }
    }

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
