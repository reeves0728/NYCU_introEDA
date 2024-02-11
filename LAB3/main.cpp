#include <iostream>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include "cudd.h"
#include "util.h"
#include "config.h"

#include <vector>
#include <map>
#define INT_MAX 2147483647

using namespace std;
typedef struct _operand
{
    int ID;
    bool isNot;
} operand;

typedef struct _data
{
    vector<vector<operand>> list;
    vector<vector<int>> order;
    vector<map<int, int>> index;
} data;

void readfile(ifstream &File, data *newdata);
void printRawData(data rawdata);
// void write_dd(DdNode *dd, string *filename);
void buildBDD(data newdata, int Order);

DdManager *gbm;
DdNode *bdd;

int main(int argc, char *argv[])
{
    string input = *(argv + 1);
    string output = *(argv + 2);
    ifstream File;
    File.open(input);
    ofstream out(output);
    // if (File.fail())
    // {
    //     cout << "input file openning fail!";
    //     exit(1);
    // }
    data newData;
    // read file
    readfile(File, &newData);
    int minNode = INT_MAX;
    // printRawData(newData);
    for (int i = 0; i < (int)newData.order.size(); i++)
    {
        // char filename[30];
        buildBDD(newData, i);
        bdd = Cudd_BddToAdd(gbm, bdd);
        // sprintf(filename, "%d_order.dot", i+1);
        // write_dd(gbm, bdd, filename);
        int temp = Cudd_DagSize(bdd);
        minNode = (temp < minNode) ? temp : minNode;
        Cudd_Quit(gbm);
    }

    out << minNode << endl;
    out.close();
    return 0;
}

void readfile(ifstream &File, data *newdata)
{
    string buffer;
    char ignore;
    File >> buffer;
    vector<vector<operand>> tempList;
    while (!buffer.empty())
    {
        // cout << "in buffer not empty!" << endl;
        vector<operand> List;
        size_t dot_pos = buffer.find('.');
        size_t plus_pos = buffer.find('+');
        size_t token_len = (dot_pos == string::npos) ? buffer.size() : min(dot_pos, plus_pos);
        std::string token = buffer.substr(0, token_len);
        for (int i = 0; i < (int)token.length(); i++)
        {
            operand temp;
            if (isupper(token[i]))
            {
                temp.ID = int(token[i] - 'A');
                temp.isNot = true;
                List.push_back(temp);
            }
            else if (islower(token[i]))
            {
                // cout << "I'm in lower!" << endl;
                temp.ID = int(token[i] - 'a');
                temp.isNot = false;
                List.push_back(temp);
            }
        }
        tempList.push_back(List);

        if (plus_pos != string::npos)
        {
            buffer = buffer.substr(plus_pos + 1);
        }
        else
        {
            buffer = (dot_pos == string::npos) ? "" : buffer.substr(dot_pos + 1);
        }
    }
    newdata->list = tempList;

    vector<vector<int>> order;
    vector<map<int, int>> index;

    while (File >> buffer)
    {
        vector<int> tempOrder;
        map<int, int> tempMap;
        for (int i = 0; i < (int)(buffer.length() - 1); i++)
        {
            tempOrder.push_back(int(buffer[i] - 'a'));
            tempMap.insert(pair<int, int>(int(buffer[i] - 'a'), i));
        }
        order.push_back(tempOrder);
        index.push_back(tempMap);
    }
    newdata->order = order;
    newdata->index = index;
}

void printRawData(data rawdata)
{
    printf("Literals:\n");
    for (int i = 0; i < (int)rawdata.list.size(); i++)
    {
        printf("%dth literal:\n", i);
        for (int j = 0; j < (int)rawdata.list[i].size(); j++)
        {
            printf("\treference ASCII: %d, isNegative: %d\n", rawdata.list[i][j].ID, rawdata.list[i][j].isNot);
        }
    }

    printf("VarOrder:\n");
    for (int i = 0; i < (int)rawdata.order.size(); i++)
    {
        for (int j = 0; j < (int)rawdata.order[i].size(); j++)
        {
            printf("%d ", rawdata.order[i][j]);
        }
        printf("\n");
    }
    printf("\n");

    printf("VarIndex\n");
    for (int i = 0; i < (int)rawdata.index.size(); i++)
    {
        for (const auto &s : rawdata.index[i])
        {
            printf("reference Ascii: %d at index = %d\n", s.first, s.second);
        }
        printf("\n");
    }
}

// void write_dd(DdNode *dd, string *filename)
// {
//     FILE *outfile;
//     outfile = fopen(filename, "w");
//     DdNode **ddnodearray = (DdNode **)malloc(sizeof(DdNode *));
//     ddnodearray[0] = dd;
//     Cudd_DumpDot(gbm, 1, ddnodearray, NULL, NULL, outfile);
//     free(ddnodearray);
//     fclose(outfile);
// }

void buildBDD(data newdata, int Order)
{
    gbm = Cudd_Init(0, 0, CUDD_UNIQUE_SLOTS, CUDD_CACHE_SLOTS, 0);
    bdd = Cudd_ReadLogicZero(gbm);
    Cudd_Ref(bdd);

    for (const auto &Literal : newdata.list)
    {
        DdNode *term = Cudd_ReadOne(gbm);
        Cudd_Ref(term);

        for (const auto &operand : Literal)
        {
            int varIndex = newdata.index[Order][operand.ID];
            DdNode *Node = operand.isNot ? Cudd_Not(Cudd_bddIthVar(gbm, varIndex)) : Cudd_bddIthVar(gbm, varIndex);
            DdNode *tempNode = Cudd_bddAnd(gbm, Node, term);
            Cudd_Ref(tempNode);
            Cudd_RecursiveDeref(gbm, term);
            term = tempNode;
        }

        DdNode *tempNode = Cudd_bddOr(gbm, term, bdd);
        Cudd_Ref(tempNode);
        Cudd_RecursiveDeref(gbm, bdd);
        bdd = tempNode;

        Cudd_RecursiveDeref(gbm, term);
    }
}
