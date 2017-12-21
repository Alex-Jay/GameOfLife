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
//---------------------------------------TODO----------------------------------------------
// 1] Add multi-threading to Update and Draw methods & use mutex locks to synchronize both operations
// 2] Add color to characters to determine survival rate
// 3] Allow user to draw their own cell pattern.
// 4] Set Console size relative to map size.

#include "stdafx.h"
#include "Utility.h" // Utility Functions
#include "Constants.h"
#include <iostream>
#include <string>
#include <vector>
#include <random> // bernoulli_distribution & default_random_engine

using namespace std;
using namespace Utility;

struct Cell
{
	int x, y; // Cell X, Y coordiantes
	bool IsAlive; // Cell life state

	string ToString()
	{
		return "X: " + to_string(x) + "\tY" + to_string(y) + "\tAlive State: " + to_string(IsAlive);
	}

	void Die()
	{
		IsAlive = false;
	}

	void Resurrect()
	{
		IsAlive = true;
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

// Populate map with random cells
void PopulateMap(vector<Cell> &map)	
{
	bool lifeState;

	for (int y = 0; y < MAP_HEIGHT; y++)
	{
		for (int x = 0; x < MAP_HEIGHT; x++)
		{
			//lifeState = rand() % 2; // Set initial life state. Range is binary [0,1]. 0 = Dead, 1 = Alive
			lifeState = RandomBooleanGenerator(); // Probability of TRUE = 20%. Set in Constants.h

			Cell cell = { x, y, lifeState }; // Construct a cell with all values. [ X Position, Y Position, Alive For # Gens, Life State ]
			map.push_back(cell); // Push Cell into vector
		}
	}
}

// Get Cell at given X, Y position
Cell GetCellAtXY(int x, int y, vector<Cell> &map)
{
	Cell retrievedCell = { 0, 0, false }; // Create a default return cell

	for (auto cell : map) // Iterate through all cells in the map
	{
		if (cell.x == x && cell.y == y) // If Cell is found at coordinate X, Y
			retrievedCell = cell; // Set found Cell to return Cell
	}

	return retrievedCell;
}

/* Count Adjacent cells

Long version of counting adjacent cells.
TODO: Clean up code.

Example:

0 = Dead Cell
1 = Alive Cell
X = Current Cell

	1	0	1
	0	X	0
	1	1	0

Return would be 4 in this case. Since There are 4 alive cells surround the current cell (X).

The function doesn't consider the current cell's life state.

*/
int GetAdjacentCellCount(Cell &currentCell, vector<Cell> &map)
{
	int aliveCount = 0;

	int currentX = currentCell.x;
	int currentY = currentCell.y;

	//vector<Cell> adjacentCells; // Create temporary vector with all adjacent cells
	//
	//adjacentCells.push_back(GetCellAtXY(currentX - 1, currentY - 1, map));	// - - // TOP LEFT CELL
	//adjacentCells.push_back(GetCellAtXY(currentX, currentY - 1, map));		// 0 - // TOP MIDDLE CELL
	//adjacentCells.push_back(GetCellAtXY(currentX + 1, currentY - 1, map));	// + - // TOP RIGHT CELL
	//adjacentCells.push_back(GetCellAtXY(currentX + 1, currentY, map));		// + 0 // MIDDLE LEFT CELL
	//adjacentCells.push_back(GetCellAtXY(currentX + 1, currentY + 1, map));	// + + // MIDDLE RIGHT CELL
	//adjacentCells.push_back(GetCellAtXY(currentX, currentY + 1, map));		// 0 + // BOTTOM LEFT CELL
	//adjacentCells.push_back(GetCellAtXY(currentX - 1, currentY + 1, map));	// - + // BOTTOM MIDDLE CELL
	//adjacentCells.push_back(GetCellAtXY(currentX - 1, currentY, map));		// - - // BOTTOM RIGHT CELL

	//for (auto adjCell : adjacentCells) // Iterate through all adjacent cells
	//{
	//	if (adjCell.IsAlive == true) // Count how many are alive
	//		aliveCount++;
	//}

	if (GetCellAtXY(currentX - 1, currentY - 1, map).IsAlive)  // - - // TOP LEFT CELL
		aliveCount++;
	if (GetCellAtXY(currentX, currentY - 1, map).IsAlive);    // 0 - // TOP MIDDLE CELL
		aliveCount++;
	if (GetCellAtXY(currentX + 1, currentY - 1, map).IsAlive)  // + - // TOP RIGHT CELL
		aliveCount++;
	if (GetCellAtXY(currentX + 1, currentY, map).IsAlive)     // + 0 // MIDDLE LEFT CELL
		aliveCount++;
	if (GetCellAtXY(currentX + 1, currentY + 1, map).IsAlive)  // + + // MIDDLE RIGHT CELL
		aliveCount++;
	if (GetCellAtXY(currentX, currentY + 1, map).IsAlive)    // 0 + // BOTTOM LEFT CELL
		aliveCount++;
	if (GetCellAtXY(currentX - 1, currentY + 1, map).IsAlive)  // - + // BOTTOM MIDDLE CELL
		aliveCount++;
	if (GetCellAtXY(currentX - 1, currentY, map).IsAlive)    // - - // BOTTOM RIGHT CELL
		aliveCount++;

	return aliveCount;
}

/* 
TODO: Encapsulate IF Statements

Game Rules
1: Any Live Cell which has < 2 Live Neighbours, die [ Underpopulation ]
2: Any Live Cell which has 2 OR 3 Neighbours, live
3: Any Live Cell which has > 3 Neighbours, die [ Overpopulation ]
4: Any Dead Cell which has EXACTLY 3 Neighbours, resurrect [ Reporduciton ]

*/
void CalculateNextGeneration(vector<Cell> &map)
{
	for (auto& cell : map) // Iterate through all cells as references
	{
		if ((cell.IsAlive && GetAdjacentCellCount(cell, map) < 2) || (cell.IsAlive && GetAdjacentCellCount(cell, map) > 3)) // If current cell has < 2 neighbours OR current cell has > 3 neighbours, die [ Underpopulation & Overpopulation ] 
		{
			// Die
			cell.Die();
		}
		if (cell.IsAlive && GetAdjacentCellCount(cell, map) == 2 || GetAdjacentCellCount(cell, map) == 3) // If current cell has 2 OR 3 adjacent neighbours, live until next generation.
		{
			// Live Until Next Generation
		}
		if (!cell.IsAlive && GetAdjacentCellCount(cell, map) == 3) // If current cell has EXACTLY 3 adjacent neighbours, resurrect [ Reproduciton ]
		{
			// Resurrect
			cell.Resurrect();
		}
	}
}

// Draw Cells
void Draw()
{
	for (auto cell : CellMap)										// Iterate through all cells in CellMap vector
	{
		// Draw Cell if Alive
		if (cell.IsAlive)			// If a cell was alive upto 10th generation, display cell as '1'.
			GotoXY(cell.x, cell.y, AliveCell);
		else														// If a cell is dead, display cell as ' '.
			GotoXY(cell.x, cell.y, DeadCell);
	}
}

// Update Cells
void Update()
{
	// Update Cells
	CalculateNextGeneration(CellMap);
}

// Start Simulation
void Start()
{
	bool isRunning = true;
	int generation = 0;

	while (isRunning)
	{
		// If Escape was Pressed.
		if (GetAsyncKeyState(27))
		{
			GotoXY(0, MAP_HEIGHT + 1, "Stopping Simulation.");
			cout << endl;
			system("PAUSE");
			break;
		}

		// Display Current Generation
		GotoXY(0, MAP_HEIGHT, "Current Generation: " + to_string(generation));

		Update();

		Draw();

		// Restrain Updates
		//Sleep(10);

		generation++;
	}
}

int main()
{
	SetConsoleSize(800, 600);

	PopulateMap(CellMap); // Initialize first generation as randomly chosen cells
	
	// Start Simulation
	Start();

    return 0;
}

