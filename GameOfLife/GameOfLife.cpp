// 
// Author: Aleksandrs Jevdokimovs
// Title: Conway's Game of Life
// Description: Game of Life using C++ console.
//
// Game Rules
// 1: Any Live Cell which has < 2 Live Neighbours, die [ Underpopulation ]
// 2: Any Live Cell which has 2 OR 3 Neighbours, live
// 3: Any Live Cell which has > 3 Neighbours, die [ Overpopulation ]
// 4: Any Dead Cell which has EXACTLY 3 Neighbours, resurrect [ Reporduciton ]
//
//----------------------TODO----------------------------------------STATUS---------------------
//
// 1] Add multi-threading to Update and Draw methods.				*DONE* 
// 2] Add color to characters to determine survival rate		 *INCOMPLETE*
// 3] Allow user to draw their own cell pattern.				 *INCOMPLETE*
// 4] Set Console size relative to map size.					 *INCOMPLETE*
// 5] Add EventDispatcher to draw onLifeStateChange Event		 *INCOMPLETE*
//

#include "stdafx.h"
#include "Utility.h" // Utility Functions
#include "Constants.h"
#include <iostream>
#include <string>
#include <vector>
#include <random> // bernoulli_distribution & default_random_engine
#include <mutex>
#include <thread>


using namespace std;

bool DEBUGGER_ENABLED = true;
bool isRunning = false;
bool ESC_wasPressed = false;
//bool isDrawing = true;
//bool isUpdating = true;
//bool isListeningInput = true;

// Thread lock
mutex threadLock;

// Current Generation
int generation = 0;

// List of ptrs to threads
vector<thread*> thread_ptrs;

struct FieldSize
{
	int x, y;
};

FieldSize fieldSize{ MAP_WIDTH, MAP_HEIGHT };

struct Cell
{
	int x, y; // Cell X, Y coordiantes
	bool IsAlive; // Cell life state

	string ToString()
	{
		return "X: " + to_string(x) + "\tY: " + to_string(y) + "\tAlive State: " + to_string(IsAlive);
	}

	void Die()
	{
		IsAlive = false;
	}

	void Resurrect()
	{
		IsAlive = true;
	}

	bool HasTopNeighbour()
	{
		return y != 0;
	}

	bool HasLeftNeighbour()
	{
		return x != 0;
	}

	bool HasRightNeighbour()
	{
		return x != fieldSize.x - 1;
	}

	bool HasBottomNeighbour()
	{
		return y != fieldSize.y - 1;
	}

	bool HasTopLeftNeighbour()
	{
		return HasLeftNeighbour() && HasTopNeighbour();
	}

	bool HasTopRightNeighbour()
	{
		return HasRightNeighbour() && HasTopNeighbour();
	}

	bool HasBottomLeftNeighbour()
	{
		return HasLeftNeighbour() && HasBottomNeighbour();
	}

	bool HasBottomRightNeighbour()
	{
		return HasRightNeighbour() && HasBottomNeighbour();
	}

	int GetTopNeighbour()
	{
		return x + (y - 1) * fieldSize.x;
	}

	int GetLeftNeighbour()
	{
		return (x - 1) + y * fieldSize.x;
	}

	int GetRightNeighbour()
	{
		return (x + 1) + y * fieldSize.x;
	}

	int GetBottomNeighbour()
	{
		return x + (y + 1) * fieldSize.x;
	}

	int GetTopLeftNeighbour()
	{
		return (x - 1) + (y - 1) * fieldSize.x;
	}

	int GetTopRightNeighbour()
	{
		return (x + 1) + (y - 1) * fieldSize.x;
	}

	int GetBottomLeftNeighbour()
	{
		return (x - 1) + (y + 1) * fieldSize.x;
	}

	int GetBottomRightNeighbour()
	{
		return (x + 1) + (y + 1) * fieldSize.x;
	}
};

vector<Cell> CellMap;

// Population Generator Values
default_random_engine generator; // Random Number Generator

// Cell Icons
string DeadCell = " ";
char AliveCell = (char)254; // White Square Unicode Value


// Generates a random boolean with a probability
bool RandomBooleanGenerator()
{
	bool ReturnState = false;

	bernoulli_distribution distribution(INITIAL_ALIVE_PROBABILITY); // Set Probability for Generator TRUE outcome

	if (distribution(generator)) // If generator is TRUE return TRUE
		ReturnState = true;

	return ReturnState; // Return FALSE by default
}

// Get Cell at given X, Y position
Cell GetCellAtXY(int x, int y, vector<Cell> &map)
{
	Cell retrievedCell = { 0, 0, false }; // Create a default return cell

	// If cell coordinate is outside of bounds, place cursor perpenciually to opposite wall.
	if (x < 0)
	{
		x += MAP_WIDTH;
	}
	else if (x > MAP_WIDTH)
	{
		x -= MAP_WIDTH;
	}
	else if (y < 0)
	{
		y += MAP_HEIGHT;
	}
	else if (y > MAP_HEIGHT)
	{
		y -= MAP_HEIGHT;
	}

	for (auto cell : map) // Iterate through all cells in the map
	{
		if (cell.x == x && cell.y == y) // If Cell is found at coordinate X, Y
			retrievedCell = cell; // Set found Cell to return Cell
	}

	return retrievedCell;
}

// Populate map with random cells
void PopulateMap(vector<Cell> &map)	
{
	bool lifeState;
	Cell cell = {};

	// Populate Map with Cells
	for (int y = 0; y < fieldSize.y; y++)
	{
		for (int x = 0; x < fieldSize.x; x++)
		{
			lifeState = RandomBooleanGenerator(); // Probability of TRUE = 20%

			cell = { x, y, lifeState }; // Construct a cell with all values. [ X Position, Y Position, Alive For # Gens, Life State ]

			map.push_back(cell); // Push Cell into vector
		}
		// Clear Cell
		cell = {};
	}
}

/* Count Adjacent cells

Example:

0 = Dead Cell
1 = Alive Cell
X = Current Cell

	1	0	1
	0	X	0
	1	1	0

Return would be 4 in this case. Since There are 4 alive cells surround the current cell (X).

The function doesn't the current cell's life state.

*/
int GetAdjacentCellCount(Cell &currentCell, vector<Cell> &map)
{
	int aliveCount = 0;
	if (currentCell.HasTopNeighbour() && map.at(currentCell.GetTopNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasTopLeftNeighbour() && map.at(currentCell.GetTopLeftNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasLeftNeighbour() && map.at(currentCell.GetLeftNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasBottomLeftNeighbour() && map.at(currentCell.GetBottomLeftNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasBottomNeighbour() && map.at(currentCell.GetBottomNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasBottomRightNeighbour() && map.at(currentCell.GetBottomRightNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasRightNeighbour() && map.at(currentCell.GetRightNeighbour()).IsAlive)
	{
		aliveCount++;
	}
	if (currentCell.HasTopRightNeighbour() && map.at(currentCell.GetTopRightNeighbour()).IsAlive)
	{
		aliveCount++;
	}

	return aliveCount;
}

/* 

Game Rules
1: Any Live Cell which has < 2 Live Neighbours, die [ Underpopulation ]
2: Any Live Cell which has 2 OR 3 Neighbours, live
3: Any Live Cell which has > 3 Neighbours, die [ Overpopulation ]
4: Any Dead Cell which has EXACTLY 3 Neighbours, resurrect [ Reporduciton ]

*/
void CalculateNextGeneration(vector<Cell> &map)
{
	for (vector<Cell>::iterator it = CellMap.begin(); it != CellMap.end(); ++it) // Iterate through all cells as references
	{
		if (((*it).IsAlive && GetAdjacentCellCount((*it), map) < 2) || ((*it).IsAlive && GetAdjacentCellCount((*it), map) > 3)) // If current cell has < 2 neighbours OR current cell has > 3 neighbours, die [ Underpopulation & Overpopulation ] 
		{
			// Die
			(*it).Die();
		}
		//if ((*it).IsAlive && GetAdjacentCellCount((*it), map) == 2 || (*it).IsAlive && GetAdjacentCellCount((*it), map) == 3) // If current cell has 2 OR 3 adjacent neighbours, live until next generation.
		//{
		//	// Live Until Next Generation
		//}
		if (!(*it).IsAlive && GetAdjacentCellCount((*it), map) == 3) // If current cell has EXACTLY 3 adjacent neighbours, resurrect [ Reproduciton ]
		{
			// Resurrect
			(*it).Resurrect();
		}
	}
}

// Draw Cells
void Draw()
{
	while (isRunning)
	{
		// Iterate through all cells in CellMap vector
		//for (auto cell : CellMap)
		for (vector<Cell>::iterator it = CellMap.begin(); it != CellMap.end(); ++it)
		{
			int currentCellXPosition = (*it).x;
			int currentCellYPosition = (*it).y;
			// Draw Cell if Alive
			if ((*it).IsAlive)
				Utility::GotoXY(currentCellXPosition, currentCellYPosition + 2, AliveCell);
			// If a cell is dead, display cell as ' '.
			else
				Utility::GotoXY(currentCellXPosition, currentCellYPosition + 2, DeadCell);
		}

		if (ESC_wasPressed)
		{
			Utility::GotoXY(0, MAP_HEIGHT + 3, "Stopping Simulation... ");
		}

		if (DEBUGGER_ENABLED)
		{
			// Display Current Generation
			Utility::GotoXY(0, 0, "Current Generation: " + to_string(generation));
			generation++;

			// Debug Cell Count
			//Utility::GotoXY(0, MAP_HEIGHT + 1, "Adjacent Cell Count : " + to_string(GetAdjacentCellCount(GetCellAtXY(4, 2, CellMap), CellMap)));

			//int neighbourLoc = GetCellAtXY(4, 2, CellMap).GetBottomRightNeighbour();
			//Utility::GotoXY(0, MAP_HEIGHT + 3, "Bottom Right Neighbour : " + CellMap.at(neighbourLoc).ToString());
		}
	}
}

// Update Cells
void Update()
{
	while (isRunning)
	{
		// Update Cells
		CalculateNextGeneration(CellMap);
	}
}

void Stop(vector<thread*> x)
{
	isRunning = false;

	// Destroy Threads
	for (vector<thread*>::iterator it = x.begin(); it != x.end(); ++it)
	{
		(*it) = NULL;
	}
}

void KeyListener()
{
	while (isRunning)
	{
		if (GetAsyncKeyState(27))
		{
			threadLock.lock();
			ESC_wasPressed = true;
			Stop(thread_ptrs);
			ESC_wasPressed = false;
			threadLock.unlock();
		}
	}
}

// Start Simulation
void Start()
{
	bool isFirstCycle = true;
	isRunning = true;

	if (isFirstCycle)
	{
		isFirstCycle = false;
		thread inputThread(KeyListener);
		thread updateThread(Update);
		thread drawThread(Draw);

		thread_ptrs.push_back(&inputThread);
		thread_ptrs.push_back(&updateThread);
		thread_ptrs.push_back(&drawThread);

		inputThread.detach();
		updateThread.join();
		drawThread.join();
	}
}
int main()
{
	ios_base::sync_with_stdio(false);

	Utility::SetConsoleSize(800, 600);

	PopulateMap(CellMap); // Initialize first generation as randomly chosen cells

	// Start Simulation
	Start();

    return 0;
}

