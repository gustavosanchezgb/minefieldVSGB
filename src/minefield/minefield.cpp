#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <algorithm>
#include <iomanip>
#include <functional>

struct Position
{
    unsigned int column = 0;
    unsigned int row = 0;
};

struct Player
{
    bool isHuman = true;
    std::string name = "Player";
    unsigned int remainingMines = 0;
    std::vector<Position> currentMines;
    std::vector<Position> currentGuesses;
};

using CellFlagsType = unsigned int;

enum class CellStatusFlags : CellFlagsType
{
    None = 0,
    Disabled = 0x01,
    HasMine = 0x02,
    WasGuessed = 0x04,
    SelfDetonated = 0x08,
    HadCollision = 0x10
};

inline CellStatusFlags operator|(CellStatusFlags a, CellStatusFlags b)
{
    return static_cast<CellStatusFlags>(static_cast<CellFlagsType>(a) | static_cast<CellFlagsType>(b));
}

inline CellStatusFlags &operator|=(CellStatusFlags &a, CellStatusFlags b)
{
    a = a | b;
    return a;
}

inline CellStatusFlags operator&(CellStatusFlags a, CellStatusFlags b)
{
    return static_cast<CellStatusFlags>(static_cast<CellFlagsType>(a) & static_cast<CellFlagsType>(b));
}

inline CellStatusFlags operator~(CellStatusFlags a)
{
    return static_cast<CellStatusFlags>(~static_cast<CellFlagsType>(a));
}

inline bool hasFlag(CellStatusFlags var, CellStatusFlags flag)
{
    using T = CellFlagsType;
    if (flag == CellStatusFlags::None)
    {
        return (var == CellStatusFlags::None);
    }
    return (static_cast<T>(var) & static_cast<T>(flag)) == static_cast<T>(flag);
}

class Board
{
public:
    static constexpr int kMaxSize = 4;
    static constexpr int kMinSize = 2;
    static constexpr int kMaxMines = 5;
    static constexpr int kMinMines = 1;

    Board(unsigned int w, unsigned int h);

    unsigned int getWidth() const;
    unsigned int getHeight() const;

    bool isValidPosition(unsigned int col, unsigned int row) const;
    bool isDisabled(unsigned int col, unsigned int row) const;
    bool isValidMineCount(unsigned int count) const;

    CellStatusFlags getCellStatus(unsigned int col, unsigned int row) const;

    void safeCellAccess(unsigned int col, unsigned int row, const std::function<void(CellStatusFlags &)> &onValidCell);

private:
    unsigned int width;
    unsigned int height;
    std::vector<std::vector<CellStatusFlags>> grid;
};

Board::Board(unsigned int w, unsigned int h)
{
    width = (w >= kMinSize && w <= kMaxSize) ? w : kMinSize;
    height = (h >= kMinSize && h <= kMaxSize) ? h : kMinSize;
    grid.assign(width, std::vector<CellStatusFlags>(height, CellStatusFlags::None));
}

unsigned int Board::getWidth() const
{
    return width;
}

unsigned int Board::getHeight() const
{
    return height;
}

bool Board::isValidPosition(unsigned int col, unsigned int row) const
{
    return (col < width && row < height);
}

bool Board::isDisabled(unsigned int col, unsigned int row) const
{
    if (!isValidPosition(col, row))
    {
        return false;
    }
    else
    {
        return hasFlag(grid.at(col).at(row), CellStatusFlags::Disabled);
    }
}

bool Board::isValidMineCount(unsigned int count) const
{
    return (count >= kMinMines && count <= kMaxMines);
}

CellStatusFlags Board::getCellStatus(unsigned int col, unsigned int row) const
{
    if (!isValidPosition(col, row))
    {
        return CellStatusFlags::None;
    }
    return grid.at(col).at(row);
}

void Board::safeCellAccess(unsigned int col, unsigned int row, const std::function<void(CellStatusFlags &)> &onValidCell)
{
    if (isValidPosition(col, row))
    {
        onValidCell(grid[col][row]);
    }
}

// board display
char getSymbolForStatus(const CellStatusFlags status)
{
    if (hasFlag(status, CellStatusFlags::SelfDetonated))
    {
        return '#';
    }
    if (hasFlag(status, CellStatusFlags::HadCollision))
    {
        return '*';
    }
    if (hasFlag(status, CellStatusFlags::WasGuessed) && hasFlag(status, CellStatusFlags::HasMine))
    {
        return 'G';
    }
    if (hasFlag(status, CellStatusFlags::Disabled))
    {
        return 'X';
    }
    return '.'; // empty
}

std::ostream &operator<<(std::ostream &stream, const Board &board)
{
    stream << "\n === BOARD === \n   ";
    for (unsigned int c = 0; c < board.getWidth(); ++c)
    {
        stream << std::setw(3) << c + 1;
    }
    stream << '\n';

    for (unsigned int r = 0; r < board.getHeight(); ++r)
    {
        stream << std::setw(3) << r + 1;
        for (unsigned int c = 0; c < board.getWidth(); ++c)
        {
            const CellStatusFlags status = board.getCellStatus(c, r);
            stream << std::setw(3) << getSymbolForStatus(status);
        }
        stream << '\n';
    }
    
    stream << '\n';
    return stream;
}

namespace utils
{
    template <typename OnValidCellFnT>
    void safeCellAccess(Board &board, unsigned int col, unsigned int row, OnValidCellFnT onValidCell)
    {
        board.safeCellAccess(col, row, std::function<void(CellStatusFlags &)>(onValidCell));
    }

    void clearInput()
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    inline void initializeRandom()
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
    }

    bool samePosition(const Position &a, const Position &b)
    {
        return ((a.column == b.column) && (a.row == b.row));
    }

    Position generateRandomPosition(const Board &board)
    {
        Position pos;
        bool found = false;
        while (!found)
        {
            pos.column = rand() % board.getWidth();
            pos.row = rand() % board.getHeight();
            if (!board.isDisabled(pos.column, pos.row))
            {
                found = true;
            }
        }
        return pos;
    }

    std::vector<Position> removeCollidingMines(const std::vector<Position> &ownMines, const std::vector<Position> &opponentMines, std::vector<Position> &collisions, Board &board)
    {
        std::vector<Position> result;
        for (const auto &mine : ownMines)
        {
            bool found = false;

            for (const auto &oppMine : opponentMines)
            {
                if (utils::samePosition(mine, oppMine))
                {
                    found = true;
                    collisions.push_back(mine);
                    utils::safeCellAccess(board, mine.column, mine.row, [](CellStatusFlags &status)
                        {
                            status |= CellStatusFlags::HadCollision;
                            status |= CellStatusFlags::Disabled;
                            status = status & ~CellStatusFlags::HasMine; 
                        });
                    break;
                }
            }
            if (!found)
            {
                result.push_back(mine);
            }
        }
        return result;
    }

    std::vector<Position> keepNonCollidingMines(const std::vector<Position> &ownMines, const std::vector<Position> &opponentMines)
    {
        std::vector<Position> result;

        for (const auto &mine : ownMines)
        {
            bool found = false;
            for (const auto &oppMine : opponentMines)
            {
                if (samePosition(mine, oppMine))
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                result.push_back(mine);
            }
        }
        return result;
    }

    Position requestPosition(const std::string &prompt, const Board &board)
    {
        unsigned int col = 0;
        unsigned int row = 0;
        bool valid = false;

        while (!valid)
        {
            std::cout << prompt << " --> [column] [row] \nInput example: 2 5\n> ";
            std::cin >> col >> row;
            if (std::cin.fail())
            {
                std::cout << "Invalid input.\n";
                clearInput();
                continue;
            }
            col--;
            row--;
            if (!board.isValidPosition(col, row) || board.isDisabled(col, row))
            {
                std::cout << "\nPosition invalid or already used.\n";
            }
            else
            {
                valid = true;
            }
        }
        return {col, row};
    }

    unsigned int chooseValidDimension(const std::string &prompt, int min_val, int max_val)
    {
        unsigned int input = 0;
        bool validInput = false;
        while (!validInput)
        {
            std::cout << prompt << " (" << min_val << "-" << max_val << ")\n> ";
            std::cin >> input;
            if (std::cin.fail())
            {
                std::cout << "\nInvalid input. Please enter a number.\n";
                clearInput();
                input = 0;
            }
            else if (input <static_cast<unsigned int>(min_val) || input> static_cast<unsigned int>(max_val))
            {
                std::cout << "\nInvalid input. Please enter a value between " << min_val << " and " << max_val << ".\n";
            }
            else
            {
                validInput = true;
            }
        }
        return input;
    }
}

namespace game
{
    void collectPositions(Player &player, int count, Board &board, std::vector<Position> &targetList, const std::string &prompt, bool showCpuMessage, bool markMinesOnBoard = false)
    {
        targetList.clear();
        std::string phaseLabel;

        if (prompt == "Guess position")
        {
            phaseLabel = "GUESSING";
        }
        else
        {
            phaseLabel = "PLACEMENT";
        }

        std::cout << "\n === " << phaseLabel << " PHASE === \n === TURN: " << player.name << " ===\n\n";

        while (targetList.size() < static_cast<size_t>(count))
        {
            Position pos;
            if (player.isHuman)
            {
                pos = utils::requestPosition(prompt, board);
            }
            else
            {
                pos = utils::generateRandomPosition(board);
            }
            bool repeated = std::any_of(targetList.begin(), targetList.end(), [&](const Position &p){ return utils::samePosition(p, pos); });
            if (!repeated)
            {
                targetList.push_back(pos);
                if (markMinesOnBoard)
                {
                    utils::safeCellAccess(board, pos.column, pos.row, [](CellStatusFlags &status){ status |= CellStatusFlags::HasMine; });
                }
                if (!player.isHuman && showCpuMessage)
                {
                    std::cout << "CPU " << (markMinesOnBoard ? "places mine" : "guesses") << " at (" << pos.column + 1 << ", " << pos.row + 1 << ")\n";
                }
            }
            else
            {
                if (player.isHuman)
                {
                    std::cout << "\nInvalid move! Position already chosen. Please choose another.\n";
                }
            }
        }
    }

    void placeMines(Player &player, int quantity, Board &board)
    {
        collectPositions(player, quantity, board, player.currentMines, "\nMine location", true, true);
    }

    void collectGuessesFromPlayer(Player &player, int opponentMines, Board &board)
    {
        collectPositions(player, opponentMines, board, player.currentGuesses, "\nGuess position", true, false);
    }

    void detectAndRemoveCollisions(Player &p1, Player &p2, Board &board)
    {
        std::vector<Position> collisions;

        std::vector<Position> newMines1 = utils::removeCollidingMines(p1.currentMines, p2.currentMines, collisions, board);
        std::vector<Position> newMines2 = utils::keepNonCollidingMines(p2.currentMines, p1.currentMines);

        int removedByP1 = p1.currentMines.size() - newMines1.size();
        int removedByP2 = p2.currentMines.size() - newMines2.size();

        p1.currentMines = newMines1;
        p2.currentMines = newMines2;

        p1.remainingMines = (p1.remainingMines >= removedByP1) ? p1.remainingMines - removedByP1 : 0;
        p2.remainingMines = (p2.remainingMines >= removedByP2) ? p2.remainingMines - removedByP2 : 0;

        for (const auto &colPos : collisions)
        {
            std::cout << "\n === MINE COLLISION IN (" << colPos.column + 1 << ", " << colPos.row + 1 << ") ===\n";
        }
        if (!collisions.empty())
        {
            std::cout << "\nMines removed - " << p1.name << ": " << removedByP1 << ", " << p2.name << ": " << removedByP2 << '\n';
        }
    }

    void clearMines(Board &board)
    {
        for (unsigned int c = 0; c < board.getWidth(); ++c)
        {
            for (unsigned int r = 0; r < board.getHeight(); ++r)
            {
                utils::safeCellAccess(board, c, r, [](CellStatusFlags &status){ status = status & ~CellStatusFlags::HasMine; });
            }
        }
    }

    int countHits(const Player &defender, const std::vector<Position> &attacks)
    {
        int hits = 0;
        for (const auto &guess : attacks)
        {
            for (const auto &mine : defender.currentMines)
            {
                if (utils::samePosition(guess, mine))
                {
                    hits++;
                    break;
                }
            }
        }
        return hits;
    }

    int resolveSelfDetonation(Player &player, Board &board)
    {
        int selfHits = 0;
        std::vector<Position> updatedMines;

        for (const auto &mine : player.currentMines)
        {
            bool destroyed = false;
            for (const auto &guess : player.currentGuesses)
            {
                if (utils::samePosition(mine, guess))
                {
                    destroyed = true;
                    break;
                }
            }

            if (destroyed)
            {
                selfHits++;
                utils::safeCellAccess(board, mine.column, mine.row, [](CellStatusFlags &status)
                    {
                        status |= CellStatusFlags::Disabled;
                        status |= CellStatusFlags::SelfDetonated;
                        status = status & ~CellStatusFlags::HasMine;
                    });
                std::cout << player.name << " exploded their own mine at (" << (mine.column + 1) << ", " << (mine.row + 1) << ")!\n";
            }
            else
            {
                updatedMines.push_back(mine);
            }
        }
        player.currentMines = updatedMines;
        return selfHits;
    }

    void disableGuessedPositions(const std::vector<Position> &guesses, Board &board)
    {
        for (const auto &guess : guesses)
        {
            utils::safeCellAccess(board, guess.column, guess.row, [](CellStatusFlags &status){ status |= CellStatusFlags::Disabled | CellStatusFlags::WasGuessed; });
        }
    }

    bool checkGameEnd(const Player &p1, const Player &p2)
    {
        if (p1.remainingMines <= 0 && p2.remainingMines <= 0)
        {
            std::cout << "\n=========================\n=== DRAW: NO MINES ===\n=========================\n";
            return true;
        }
        else if (p1.remainingMines <= 0)
        {
            std::cout << "\n==================================\n=== " << p2.name << " WIN THE GAME! ===\n==================================\n";
            return true;
        }
        else if (p2.remainingMines <= 0)
        {
            std::cout << "\n==================================\n=== " << p1.name << " WIN THE GAME! ===\n==================================\n";
            return true;
        }
        return false;
    }

    void runMainLoop(Player &p1, Player &p2, Board &board)
    {
        int round = 1;
        bool finished = false;

        while (!finished)
        {
            std::cout << "\n===============\n=== ROUND " << round << " ===\n===============\n";
            std::cout << board;

            clearMines(board);
            placeMines(p1, p1.remainingMines, board);
            placeMines(p2, p2.remainingMines, board);
            detectAndRemoveCollisions(p1, p2, board);

            collectGuessesFromPlayer(p1, p2.remainingMines, board);
            collectGuessesFromPlayer(p2, p1.remainingMines, board);

            int hits1 = countHits(p2, p1.currentGuesses);
            int hits2 = countHits(p1, p2.currentGuesses);

            p2.remainingMines = (p2.remainingMines >= hits1) ? p2.remainingMines - hits1 : 0;
            p1.remainingMines = (p1.remainingMines >= hits2) ? p1.remainingMines - hits2 : 0;

            int selfHits1 = resolveSelfDetonation(p1, board);
            int selfHits2 = resolveSelfDetonation(p2, board);

            p1.remainingMines = (p1.remainingMines >= selfHits1) ? p1.remainingMines - selfHits1 : 0;
            p2.remainingMines = (p2.remainingMines >= selfHits2) ? p2.remainingMines - selfHits2 : 0;

            disableGuessedPositions(p1.currentGuesses, board);
            disableGuessedPositions(p2.currentGuesses, board);

            std::cout << "\n=== ROUND " << round << " RESULTS ===\n";
            std::cout << board;
            std::cout << p1.name << " - Remaining mines: " << p1.remainingMines << "\n";
            std::cout << p2.name << " - Remaining mines: " << p2.remainingMines << "\n";

            finished = checkGameEnd(p1, p2);
            round++;
        }
        std::cout << "\n=== GAME OVER ===\n";
    }

    bool chooseGameMode(bool &exitChosen)
    {
        unsigned int option = 0;
        exitChosen = false;

        while (option != 1 && option != 2 && option != 3)
        {
            std::cout << "1. Player vs CPU\n2. Player 1 vs Player 2\n3. Exit Game\n> ";
            std::cin >> option;

            if (std::cin.fail())
            {
                utils::clearInput();
                option = 0;
            }

            if (option != 1 && option != 2 && option != 3)
            {
                std::cout << "\nInvalid option. Enter 1, 2, or 3.\n";
            }
        }

        if (option == 3)
        {
            exitChosen = true;
            return false;
        }
        else if (option == 1)
        {
            return true;
        }
        else // option == 2
        {
            return false;
        }
    }

    int chooseMineCount(const Board &board)
    {
        unsigned int mines = 0;
        bool validInput = false;

        while (!validInput)
        {
            std::cout << "Choose the number of mines between " << Board::kMinMines << " and " << Board::kMaxMines << ".\n> ";
            std::cin >> mines;

            bool failedInput = std::cin.fail();
            bool outOfRange = !board.isValidMineCount(mines);

            if (failedInput)
            {
                std::cout << "Invalid input. Please enter a number.\n";
                utils::clearInput();
                mines = 0;
            }
            else if (outOfRange)
            {
                std::cout << "Invalid input. Please enter a value between " << Board::kMinMines << " and " << Board::kMaxMines << ".\n";
            }
            else
            {
                validInput = true;
            }
        }
        return mines;
    }

    bool askPlayAgain()
    {
        const char YES = 'y';
        const char NO = 'n';
        char option = '\0';

        std::cout << "Do you want to play again? (" << YES << "/" << NO << ")\n> ";
        std::cin >> option;

        while (std::cin.fail() || (option != YES && option != NO))
        {
            std::cout << "Invalid entry. Enter '" << YES << "' for <YES> or '" << NO << "' for <NO> \n> ";
            utils::clearInput();
            std::cin >> option;
        }
        return option == YES;
    }
}

int main()
{
    utils::initializeRandom();
    bool playAgain = true;

    while (playAgain)
    {
        std::cout << "\n======================\n=== MINEFIELD GAME ===\n======================\n";
        bool exitChosen = false;
        bool vsCPU = false;
        vsCPU = game::chooseGameMode(exitChosen);
        if (exitChosen)
        {
            break;
        }

        // board setup
        std::cout << "\n=== BOARD DIMENSIONS ===\n";
        unsigned int width = utils::chooseValidDimension("Board Width", Board::kMinSize, Board::kMaxSize);
        unsigned int height = utils::chooseValidDimension("Board Height", Board::kMinSize, Board::kMaxSize);
        Board board(width, height);
        std::cout << board;

        // mines setup
        std::cout << "=== NUMBER OF MINES ===\n";
        unsigned int mines = game::chooseMineCount(board);

        // player setup
        Player player1 = {true, "Player 1", mines};
        Player player2 = {vsCPU ? false : true, vsCPU ? "CPU" : "Player 2", mines};

        // the game
        game::runMainLoop(player1, player2, board);
        playAgain = game::askPlayAgain();
    }
    std::cout << "\nThanks for playing Minefield! See you next time.\n";
    return 0;
}
