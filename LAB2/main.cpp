#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <queue>
#include <utility>
using namespace std;
#define EMPTY 0
#define BLOCK 1
#define PIN 2
#define NET 3
#define mazeEmpty -1
#define mazeBlocked -2
#define init   \
    {          \
        -1, -1 \
    }

typedef struct coordinate
{
    int x;
    int y;
} Coordinate;

typedef struct Grid
{
    int row;
    int col;
    int **gridState;
} Grid;

typedef struct block
{
    int LeftDownX;
    int RightUpX;
    int LeftDownY;
    int RightUpY;
} Block;

typedef struct net
{
    string netName;
    int sourceX;
    int sourceY;
    int targetX;
    int targetY;
    int gridNum;
} Net;

typedef vector<vector<Coordinate>> vect2d;
typedef vector<Coordinate> vect;
void readFile(ifstream &File, int *row, int *col, int *totBlock, int *totNum, Block **blockArray, Net **netArray);
void printInputFile(int ROW, int COL, int NumBlock, Block *BlockArray, int NumNet, Net *NetArray);
void sortNetArray(Net *NetArray, int totNum);
bool compare(Net n1, Net n2);
Grid *createGrid(int row, int col, int totnum, int totBlock, Block *blockArray, Net *netArray);
// void updateGrid(vect path, Grid *grid);
void updateGrid(vect Path, Grid *grid, Net net);
vect route(Grid *grid, Net net, bool *valid);
bool vaildStep(int x, int y, Grid *grid);
void swap(int *x, int *y);
void printPath(vector<Coordinate> Path);
void initGrid(Grid *grid);
void criticalNet(Net *netArray, int totNum, int critical);
void fileOutput(ofstream &File, int totNum, vect2d pathArray, Net *netArray);

int main(int argc, char *argv[])
{
    string input = *(argv + 1);
    string output = *(argv + 2);
    ifstream File;
    ofstream out(output);
    vect2d pathArray;
    File.open(input);
    if (File.fail())
    {
        cout << "input file openning fail!";
        exit(1);
    }
    int row, col, totBlock, totNum;
    Block *blockArray;
    Net *netArray;
    Grid *grid;
    readFile(File, &row, &col, &totBlock, &totNum, &blockArray, &netArray);
    // printInputFile(row, col, totBlock, blockArray, totNum, netArray);
    sortNetArray(netArray, totNum);
    // cout << "after sortNetArray!" << endl;
    grid = createGrid(row, col, totNum, totBlock, blockArray, netArray);
    bool validRouting = false;
    while (!validRouting)
    {
        pathArray.clear();
        for (int i = 0; i < totNum; i++)
        {
            validRouting = false;
            // cout << "path " << i << endl;
            vect Path = route(grid, netArray[i], &validRouting);

            if (!validRouting)
            {
                cout << "re-Route for :" << netArray[i].netName << endl;
                initGrid(grid);
                criticalNet(netArray, totNum, i);
                break;
            }

            else
            {

                pathArray.push_back(Path);
                netArray[i].gridNum = int(Path.size()) - 2;
                // printPath(Path);
                updateGrid(pathArray[i], grid, netArray[i]);
            }
        }
    }
    fileOutput(out, totNum, pathArray, netArray);
    delete[] blockArray;
    delete[] netArray;
    delete[] grid;
    return 0;
}

void readFile(ifstream &File, int *row, int *col, int *totBlock, int *totNum, Block **blockArray, Net **netArray)
{
    char ignore;
    string dummyString;
    File >> dummyString >> *row;
    File >> dummyString >> *col;
    ignore = File.get();
    File >> dummyString >> *totBlock;
    *blockArray = new Block[*totBlock];
    for (int i = 0; i < *totBlock; i++)
    {
        File >> (*blockArray)[i].LeftDownX >> (*blockArray)[i].RightUpX >> (*blockArray)[i].LeftDownY >> (*blockArray)[i].RightUpY;
    }
    ignore = File.get();
    File >> dummyString >> *totNum;
    *netArray = new Net[*totBlock];
    for (int i = 0; i < *totNum; i++)
    {
        File >> (*netArray)[i].netName >> (*netArray)[i].sourceX >> (*netArray)[i].sourceY >> (*netArray)[i].targetX >> (*netArray)[i].targetY;
    }
}

void printInputFile(int ROW, int COL, int NumBlock, Block *BlockArray, int NumNet, Net *NetArray)
{
    cout << "Grid size <ROW> <COL>: " << ROW << " " << COL << endl;
    cout << "NumBlock <blockCount>: " << NumBlock << endl;
    for (int i = 0; i < NumBlock; i++)
    {
        cout << "\tBlock: " << i + 1 << "<LeftDownX> <RightUpX> <LeftDownY> <RightUpY>:" << BlockArray[i].LeftDownX << " " << BlockArray[i].RightUpX << " " << BlockArray[i].LeftDownY << " " << BlockArray[i].RightUpY << endl;
    }
    cout << "NumNet <netCounts>: " << NumNet << endl;
    for (int i = 0; i < NumNet; i++)
    {
        cout << "\tNet " << i + 1 << ": "
             << "<NetName> <sourceX> <sourceY> <targetX> <targetY>: " << NetArray[i].netName << " " << NetArray[i].sourceX << " " << NetArray[i].sourceY << " " << NetArray[i].targetX << " " << NetArray[i].targetY;
    }
}

void sortNetArray(Net *NetArray, int totNum)
{
    sort(NetArray, NetArray + totNum, compare);
}

bool compare(Net n1, Net n2)
{
    int n1Distance = abs(n1.sourceX - n1.targetX) + abs(n1.sourceY - n1.targetY);
    int n2Distance = abs(n2.sourceX - n2.targetX) + abs(n2.sourceY - n2.targetY);
    return n1Distance < n2Distance;
}

Grid *createGrid(int row, int col, int totnum, int totBlock, Block *blockArray, Net *netArray)
{
    Grid *newGrid = new Grid;
    newGrid->row = row;
    newGrid->col = col;
    newGrid->gridState = new int *[row];
    for (int i = 0; i < row; i++)
    {
        newGrid->gridState[i] = new int[col];
    }
    // init
    for (int i = 0; i < row; i++)
    {
        for (int j = 0; j < col; j++)
        {
            newGrid->gridState[i][j] = EMPTY;
        }
    }

    for (int i = 0; i < totBlock; i++)
    {
        int LeftDownX, RightUpX, LeftDownY, RightUpY;
        LeftDownX = blockArray[i].LeftDownX;
        RightUpX = blockArray[i].RightUpX;
        LeftDownY = blockArray[i].LeftDownY;
        RightUpY = blockArray[i].RightUpY;

        for (int y = LeftDownX; y <= RightUpX; y++)
        {
            for (int x = LeftDownY; x <= RightUpY; x++)
            {
                newGrid->gridState[x][y] = BLOCK;
            }
        }
    }
    for (int i = 0; i < totnum; i++)
    {
        int sourceX, sourceY, targetX, targetY;
        sourceX = netArray[i].sourceX;
        targetX = netArray[i].targetX;
        sourceY = netArray[i].sourceY;
        targetY = netArray[i].targetY;

        newGrid->gridState[sourceY][sourceX] = PIN;
        newGrid->gridState[targetY][targetX] = PIN;
    }

    return newGrid;
}

void updateGrid(vect Path, Grid *grid, Net net)
{
    for (int i = 0; i < Path.size(); i++)
    {
        Coordinate coord = Path[i];

        if ((coord.x == net.sourceX && coord.y == net.sourceY) || (coord.x == net.targetX && coord.y == net.targetY))
        {
            continue;
        }
        grid->gridState[coord.y][coord.x] = NET;
    }
}

vect route(Grid *grid, Net net, bool *valid)
{

    int directionX[4] = {0, -1, 0, 1};
    int directionY[4] = {1, 0, -1, 0};

    vector<vector<int>> maze(grid->row, vector<int>(grid->col, mazeEmpty));
    vector<vector<Coordinate>> previous(grid->row, vect(grid->col, init));

    for (int i = 0; i < grid->row; i++)
    {
        for (int j = 0; j < grid->col; j++)
        {
            if (grid->gridState[i][j])
            {
                maze[i][j] = mazeBlocked;
            }
        }
    }

    queue<pair<int, int>> q;
    queue<int> totDistance;
    int distance = 0;
    maze[net.sourceY][net.sourceX] = 0;
    q.push({net.sourceX, net.sourceY});
    totDistance.push(distance + 1);

    while (!q.empty())
    {
        bool finish = false;

        int x = q.front().first;
        int y = q.front().second;
        q.pop();

        int currentDistance = totDistance.front();
        totDistance.pop();

        for (int i = 0; i <= 3; i++)
        {
            int nextX = x + directionX[i];
            int nextY = y + directionY[i];

            if (!vaildStep(nextX, nextY, grid))
            {
                continue;
            }

            if (nextX == net.targetX && nextY == net.targetY)
            {
                *valid = true;
                previous[nextY][nextX].x = x;
                previous[nextY][nextX].y = y;
                finish = true;
                break;
            }

            else if (maze[nextY][nextX] == mazeEmpty)
            {
                maze[nextY][nextX] = currentDistance;
                previous[nextY][nextX].x = x;
                previous[nextY][nextX].y = y;
                q.push({nextX, nextY});
                totDistance.push(currentDistance + 1);

                swap(&directionX[0], &directionX[i]);
                swap(&directionY[0], &directionY[i]);
            }
        }
        if (finish)
            break;
    }

    vect path;
    Coordinate tempCoordinate;
    tempCoordinate.y = net.targetY;
    tempCoordinate.x = net.targetX;
    path.push_back(tempCoordinate);
    for (pair<int, int> temp = {net.targetX, net.targetY};
         previous[temp.second][temp.first].x != -1 && previous[temp.second][temp.first].y != -1;
         temp = {previous[temp.second][temp.first].x, previous[temp.second][temp.first].y})
    {
        tempCoordinate.x = previous[temp.second][temp.first].x;
        tempCoordinate.y = previous[temp.second][temp.first].y;
        path.push_back(tempCoordinate);
    }

    reverse(path.begin(), path.end());
    return path;
}

bool vaildStep(int x, int y, Grid *grid)
{
    if (x < 0 || y < 0 || x >= grid->col || y >= grid->row)
    {
        return false;
    }
    return true;
}

void swap(int *x, int *y)
{
    int temp;
    temp = *x;
    *x = *y;
    *y = temp;
}

void printPath(vector<Coordinate> Path)
{
    for (int i = 0; i < Path.size(); i++)
    {
        cout << "(" << Path[i].x << "," << Path[i].y << ") -> ";
    }
    cout << endl;
}

void initGrid(Grid *grid)
{
    for (int i = 0; i < grid->row; i++)
    {
        for (int j = 0; j < grid->col; j++)
        {
            if (grid->gridState[i][j] == NET)
            {
                grid->gridState[i][j] = EMPTY;
            }
        }
    }
}

void criticalNet(Net *netArray, int totNum, int critical)
{
    Net criticalNet = netArray[critical];
    for (int i = critical - 1; i >= 0; i--)
    {
        netArray[i + 1] = netArray[i];
    }
    netArray[0] = criticalNet;
}

void fileOutput(ofstream &File, int totNum, vect2d pathArray, Net *netArray)
{
    for (int i = 0; i < totNum; i++)
    {
        File << netArray[i].netName << " " << netArray[i].gridNum << endl;
        File << "begin" << endl;
        vect thisPath = pathArray[i];
        Coordinate thisCoordinate = thisPath[0];
        File << thisCoordinate.x << " " << thisCoordinate.y << " ";
        for (int j = 0; j < int(thisPath.size()) - 1; j++)
        {
            if ((thisPath[j + 1].x == thisCoordinate.x) || thisPath[j + 1].y == thisCoordinate.y)
            {
                continue;
            }
            else
            {
                thisCoordinate = thisPath[j];
                File << thisCoordinate.x << " " << thisCoordinate.y << endl;
                File << thisCoordinate.x << " " << thisCoordinate.y << " ";
                j--;
            }
        }
        File << netArray[i].targetX << " " << netArray[i].targetY << endl;
        File << "end" << endl;
    }

    File.close();
}