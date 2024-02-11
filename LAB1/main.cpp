#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string.h>
#include <chrono>
#include <algorithm>

#define BLANCED_FACTOR 0.45

using namespace std;
typedef vector<vector<int>> vect2d;
typedef vector<int> vect;

auto start_time = chrono::high_resolution_clock::now();

vect2d createNetArray(const string& filename, int* netNumber, int* nodeNumber);
vect2d createCellArray(const string& filename, int* maxCut);
void printNetArray(vect2d netArray);
void printCellArray(vect2d cellArray);
vect init_partition(int *leftCellCount, int *rightCellCount);
void printPartition(vect &partition, int leftPartitionCellCount, int rightPartitionCellCount);
vect calculate_gain(vect &partition, vect2d &netArray, vect2d &cellArray);
int FS(vect &partition, vect2d &netArray, vect2d &cellArray, int cellNum);
int TE(vect &partition, vect2d &netArray, vect2d &cellArray, int cellNum);
void gainList(vect &gain, vect2d &leftGainList, vect2d &rightGainList, vect &partition);
void printGainList(vect2d GainList);
void init_lockState(vect &lockState);
void printCellLockState(vect &CellLockState);
int maxCellGain(vect2d &leftGainList, vect2d &rightGainList, int leftCellCount, int rightCellCount);
void removeMaxCellGain(vect2d &leftGainList, vect2d &rightGainList, vect &gain, vect &partition ,int maxGain_id);
void updateLockState(vect &lockState, int maxGain_id);
void updatePartition(vect &partition, int *leftCellCount, int *rightCellCount, int maxGain_id);
void updateNeighbor(vect2d &leftGainList, vect2d &rightGainList, vect &gain, vect2d &netArray, vect2d &cellArray, int maxGain_id, vect &partition, vect &lockState);
void outputOpPartition(vect oppartition);

int netNum;  // the net count of the netlist
int nodeNum; // the cell count of the netlist
int maxCut;

int main(int argc, char *argv[])
{
    string input_filename(argv[1]);

    vect2d netArray;
    vect2d cellArray; 

    vect partition;
    vect opPartition;
    vect gain;

    int leftCellCount;
    int rightCellCount;

    netArray = createNetArray(input_filename, &netNum, &nodeNum);
    cellArray = createCellArray(input_filename, &maxCut);
    //index smaller than maxcut indicates minus cell count
    vect2d leftGainList(2*maxCut+1);
    vect2d rightGainList(2*maxCut+1);
    vect lockState(nodeNum+1);

    partition = init_partition(&leftCellCount,&rightCellCount);
    gain = calculate_gain(partition,netArray,cellArray);
    gainList(gain,leftGainList,rightGainList,partition);
    init_lockState(lockState);

    int opLeftCellCount = leftCellCount;
    int opRightCellCount = rightCellCount;
    int moveID;
    int totalGain = 0;
    int tempMaxGain = 0;
    opPartition = partition;
    while(true){

        for(int i = 1; i <= nodeNum; i++){
        moveID = maxCellGain(leftGainList,rightGainList,leftCellCount,rightCellCount);
        totalGain += gain[moveID];

        removeMaxCellGain(leftGainList,rightGainList,gain,partition,moveID);
        updateLockState(lockState,moveID);
        updatePartition(partition,&leftCellCount,&rightCellCount,moveID);
        updateNeighbor(leftGainList,rightGainList,gain,netArray,cellArray,moveID,partition,lockState);

        if(totalGain>=tempMaxGain){
            tempMaxGain = totalGain;
            opPartition = partition;
            opLeftCellCount = leftCellCount;
            opRightCellCount = rightCellCount;
        }
        auto end_time_once = chrono::high_resolution_clock::now();
        auto duration_once = chrono::duration_cast<chrono::microseconds>(end_time_once - start_time);
        if (int(duration_once.count())>29000000) break;
        // cout<<"========================="<<endl;
        // printPartition(partition,leftCellCount,rightCellCount);
        }
        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end_time - start_time);
        // cout<<"partition duration: "<<int(duration.count())<<endl;
        if (int(duration.count())>29500000) break;
        leftCellCount = opLeftCellCount;
        rightCellCount = opRightCellCount;
        partition = opPartition;
        totalGain = 0;
        tempMaxGain = 0;
        init_lockState(lockState);
        gainList(gain,leftGainList,rightGainList,partition);
    }
    outputOpPartition(opPartition);


    return 0;
}

void outputOpPartition(vect oppartition){
    ofstream output("output.txt");
    for(int i = 1; i <= nodeNum; i++){
        output << oppartition[i] << '\n';
    }
    output.close();
}

void updateNeighbor(vect2d &leftGainList, vect2d &rightGainList, vect &gain, vect2d &netArray, vect2d &cellArray, int maxGain_id, vect &partition, vect &lockState){
    for(const auto &k : cellArray[maxGain_id]){
        for(const auto &n : netArray[k]){
            if (lockState[n]==0 && n != maxGain_id){
                removeMaxCellGain(leftGainList,rightGainList,gain,partition,n);
                gain[n] = FS(partition, netArray, cellArray, n) - TE(partition, netArray, cellArray,n);
                if(partition[n]){
                    rightGainList[maxCut+gain[n]].push_back(n);
                }
                else{
                    leftGainList[maxCut+gain[n]].push_back(n);
                }
            }
        }
    }
}

void updatePartition(vect &partition, int *leftCellCount, int *rightCellCount, int maxGain_id){
    if(partition[maxGain_id]){
        partition[maxGain_id] = 0;
        (*leftCellCount)++;
		(*rightCellCount)--;
    }

    else{
        partition[maxGain_id] = 1;
        (*leftCellCount)--;
		(*rightCellCount)++;
    }
}

void updateLockState(vect &lockState, int maxGain_id){
    lockState[maxGain_id] = 1;
}


void removeMaxCellGain(vect2d &leftGainList, vect2d &rightGainList, vect &gain, vect &partition ,int maxGain_id){
    int removeCell = maxCut + gain[maxGain_id];
    if (partition[maxGain_id]){
        auto it = find(rightGainList[removeCell].begin(), rightGainList[removeCell].end(), maxGain_id);
        if (it != rightGainList[removeCell].end()) {
            rightGainList[removeCell].erase(it);
        }
    }
    else{
        auto it = find(leftGainList[removeCell].begin(), leftGainList[removeCell].end(), maxGain_id);
        if (it != leftGainList[removeCell].end()) {
            leftGainList[removeCell].erase(it);
        }
    }
}

int maxCellGain(vect2d &leftGainList, vect2d &rightGainList, int leftCellCount, int rightCellCount){
    int maxGain_id;
    // cout << "BLANCED_FACTOR * (float(leftCellCount+rightCellCount)/2) = "<<BLANCED_FACTOR * (float(leftCellCount+rightCellCount))<<endl;

    // must move from right to left
    if(leftCellCount< BLANCED_FACTOR * (float(leftCellCount+rightCellCount))){
        // cout<<"must move from right to left"<<endl;
        for(int i = 2*maxCut; i>=0; i--){ //find the existed max gain from the top of the gain list
            if(!rightGainList[i].empty()){
                maxGain_id = rightGainList[i].back();
                return maxGain_id;
            }
        }
    }

    // must move from left to right
    else if(rightCellCount< BLANCED_FACTOR * (float(leftCellCount+rightCellCount))){
        for(int i = 2*maxCut; i>=0; i--){ //find the existed max gain from the top of the gain list
        // cout<<"must move from left to right"<<endl;
            if(!leftGainList[i].empty()){
                maxGain_id = leftGainList[i].back();
                return maxGain_id;
            }
        }
    }

    // move cell freely
    else{
        for(int i = 2*maxCut; i>=0; i--){
            if(leftGainList[i].empty() && rightGainList[i].empty()) {
                // cout<<"if both the gain list are empty for this gain value"<<endl;
                continue;
            }

            else if(!leftGainList[i].empty() && !rightGainList[i].empty()){
                // cout<<"if both the gain list isn't empty, choose the left side first"<<endl;
				maxGain_id = rightGainList[i].back();
				return maxGain_id;
			}

            else if(!leftGainList[i].empty() && rightGainList[i].empty()){
                // cout<<"if the gainList on the left side is not empty, and the right side is empty"<<endl;
				maxGain_id = leftGainList[i].back();
				return maxGain_id;
			}

            else if(leftGainList[i].empty() && !rightGainList[i].empty()){
                // cout<<"if the gainList on the left side is empty, and the right side isn't empty"<<endl;
				maxGain_id = rightGainList[i].back();
				return maxGain_id;
			}
        }
    }
    return -1;
}

void printCellLockState(vect &lockState){
	for(int i = 1; i < (int)lockState.size(); i++){
		if(lockState[i] == 0) printf("%d cell: Unlocked\n", i);
		else printf("%d cell: Locked\n", i);
	}
}
void init_lockState(vect &lockState){
    for(int i = 1; i <= nodeNum; i++){
		lockState[i] = 0;
	}
}

void printGainList(vect2d GainList){
    int i = 2 * maxCut;
    while(i >= 0){
        cout << "Gain = " << i - maxCut << "\t:";
        for(const auto &t : GainList[i]){
            cout << t << " ";
        }
        cout << "\n";
        i--;
    }
    cout << "\n";
}

void gainList(vect &gain, vect2d &leftGainList, vect2d &rightGainList, vect &partition){
    for(int i = 1; i<= nodeNum; i++){
        if(partition[i]) {
            rightGainList[maxCut + gain[i]].push_back(i);
        }
        else {
            leftGainList[maxCut + gain[i]].push_back(i);
        }
    }
}

vect calculate_gain(vect &partition, vect2d &netArray, vect2d &cellArray){
    vect gain(nodeNum+1);
    for(int i = 1; i <= nodeNum; i++){
		gain[i] = FS(partition, netArray, cellArray, i) - TE(partition, netArray, cellArray,i);
	}
    return gain;
}

int FS(vect &partition, vect2d &netArray, vect2d &cellArray, int cellNum){
    int FS_result = 0;
    int alone =0;
    // read only
    for(const auto &i : cellArray[cellNum]){
        for(const auto &s : netArray[i]){
            alone = 1;
            if(s != cellNum){
                if(partition[s] == partition[cellNum]){
                    alone = 0;
                    break;
                }
            }
        }
        FS_result += alone;
    }
    return FS_result;
}

int TE(vect &partition, vect2d &netArray, vect2d &cellArray, int cellNum){
    int TE_result = 0;
    int allSame = 0;

    //read only
    for(const auto &i : cellArray[cellNum]){
        for(const auto &s : netArray[i]){
            allSame = 1;
            if(s != cellNum){
                if(partition[s] != partition[cellNum]){
                    allSame = 0;
                    break;
                }
            }
        }
        TE_result += allSame;
    }
    return TE_result;
}

vect init_partition(int *leftCellCount, int *rightCellCount){
	vect partition ((nodeNum + 1)); // start from index 1
	*leftCellCount = 0;
	*rightCellCount = 0;
    // 
	for(int i = 1; i <=nodeNum/2; i++){
		partition[i] = 0;
		(*leftCellCount)++;
	}

    for(int i = nodeNum/2 + 1; i <=nodeNum; i++){
		partition[i] = 1;
		(*rightCellCount)++;
	}
	return partition;
}

void printPartition(vect &partition, int leftPartitionCellCount, int rightPartitionCellCount){
	for(int i = 1; i <=nodeNum; i++){		    
        if(partition[i]==0){
            cout<<"node "<<i<<": 0(left)"<<endl;
        }
        else{
            cout<<"node "<<i<<": 1(right)"<<endl;
        }
	}
}

vect2d createNetArray(const string& filename, int* netNumber, int* nodeNumber) {
    ifstream input(filename);
    if (!input.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    input >> (*netNumber) >> (*nodeNumber);
    input.ignore();

    int netArrayindex = 1;
    vect2d netArray((*netNumber) + 1);

    string line;
    while (getline(input, line)) {
        istringstream iss(line);
        int node;
        while (iss >> node) {
            netArray[netArrayindex].push_back(node);
        }
        netArrayindex++;
    }

    input.close();
    return netArray;
}
vect2d createCellArray(const string& filename, int* maxCut)
{
    ifstream input(filename);
    if (!input.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        exit(1);
    }

    int netNumber;
    int nodeNumber;
    input >> netNumber >> nodeNumber;
    input.ignore();

    // the array to record the terminal of each cell
    vector<int> pinCount(nodeNumber + 1);
    vect2d cellArray(nodeNumber + 1);

    int netIndex = 1;
    string line;
    while (getline(input, line)) {
        istringstream iss(line);
        int node;
        while (iss >> node) {
            cellArray[node].push_back(netIndex);
            pinCount[node]++;
        }
        netIndex++;
    }
    input.close();

    (*maxCut) = 0;
    for (int i = 1; i <= nodeNumber; i++) {
        (*maxCut) = (*maxCut) < pinCount[i] ? pinCount[i] : (*maxCut);
    }
    return cellArray;
}
void printNetArray(vect2d netArray)
{
    for (int i = 1; i < (int)netArray.size(); i++)
    {
        cout << "Net number = " << i << ": ";
        for (const auto &s : netArray[i])
        {
        cout << s << " ";
        }
        cout << endl;
    }
}
void printCellArray(vect2d cellArray)
{
    for (int i = 1; i < (int)cellArray.size(); i++)
    {
        cout << "cell number = " << i << ": ";
        for (const auto &s : cellArray[i])
        {
        cout << s << " ";
        }
        cout << endl;
    }
}