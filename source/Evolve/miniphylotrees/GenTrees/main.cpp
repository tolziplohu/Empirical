
#include <iostream>
#include <vector>
#include <memory>
#include <cstdlib>
#include "../../Systematics.h"
#include "../../../tools/Random.h"
#include "../../../tools/IndexMap.h"
#include "../../../base/map.h"
#include "../../../tools/map_utils.h"

//g++ -std=c++17 -I../../ main.cpp -o main.o && ./main.o
//for i in {1..5}; do g++ -std=c++17 -I../../ main.cpp -o main.o && ./main.o; done
//for i in {1..100}; do ./main.o; done


using namespace std;

int randScope;
int numOrgs = 10;
int parentNum;
int numGens = 100;
int systime = 0;
double mutRate = 0.05;
//int genotype = 0;
int TenGens = 10;

class Organism {
public:
    //int clade;
    int genotype = 0;

    //heritable, can mutate in reproduction //replace clade with genotype
    //need mutation rate .05 -> or higher (configurable at top)
    //(.P use to determine if mutation happens)
    //how much does genotype change when it mutates
    //choose random new genotype within certain range
    //option to turn on pressure for diversity (empirical config object) (change from command line)

    Organism() {
    }

    Organism(int _genotype) {
        genotype = _genotype;
    }

    int MutateGenotype(emp::Random &RandNum) {

        double randMutation = RandNum.GetDouble(0, 1);
        //cout << "random mutation rate = " << randMutation << endl;

        //cout << "GENOTYPE BEFORE MUTATION: " << genotype << endl;

        if (randMutation < mutRate) {
            int MutatedGenotype = genotype - RandNum.GetInt(-3, 3);
            genotype = MutatedGenotype;
            //cout << "mutated genotype = " << genotype << endl;
        } else {
            //cout << "not mutated genotype = " << genotype << endl;
        }
        //cout << "GENOTYPE: " << genotype << endl;

        return genotype;
    }
};



//    static void reproduce(vector<Organism> &childGen, emp::Systematics<Organism, int> &sys){
//        for(int i = 0; i < 2; i++){
//            childGen.emplace_back(); //fills childGen vector with Organisms
//
//            //ORG & org, WorldPosition pos, int update=-1
//            emp::WorldPosition pos(i, 1);
//            sys.AddOrg(childGen[i], pos, systime); //removed brackets childGen[i], {i, 0}
//        }
//        cout << "child generation created" << endl;
//    //}


int chooseOrg(vector<Organism> &currentGen, emp::Random &randNum){

    parentNum = randNum.GetInt(numOrgs);  //chooses random spot in array for parent
    //cout << "parent chosen is in spot " << parentNum << " in currentGen array which is " << size(currentGen) << " long" << endl;
    return parentNum;
}


void calcFitness(vector<Organism> &currentGen, vector<double> &fitnessVect, emp::Random &randNum) {
    fitnessVect.resize(0);

    vector<int> fitnessCalc;

    fitnessCalc.reserve(currentGen.size());
    for (int i = 0; i < currentGen.size(); i++) {
        fitnessCalc.push_back(currentGen[i].genotype);
    }

    map<int, int> CountMap;

    for (int j = 0; j < fitnessCalc.size(); j++) {
        if (emp::Has(CountMap, fitnessCalc[j])) {
            CountMap[fitnessCalc[j]]++;
        } else {
            CountMap[fitnessCalc[j]] = 1;

        }
    }

    for(int k = 0; k < fitnessCalc.size(); k++){
        fitnessVect.push_back(1.0/CountMap[fitnessCalc[k]]);
    }
}


int chooseOrgDiversity(vector<double> &fitnessVect, emp::Random &randNum){
    emp::IndexMap fitness_index(fitnessVect.size());

    for (size_t id = 0; id < fitnessVect.size(); id++){
        fitness_index.Adjust(id, fitnessVect[id]);
    }

    const double fit_pos = randNum.GetDouble(fitness_index.GetWeight());
    size_t parent_id = fitness_index.Index(fit_pos);

    //cout << "FITNESS VECTOR VALUES: " << endl;

    for(int pos = 0; pos < fitnessVect.size(); pos++){
        //cout << fitnessVect[pos] << " " << endl;
    }

    parentNum = parent_id;

    //cout << "PARENT NUM AFTER CHOOSEORGDIVERSITY: " << parentNum << endl;
    //cout << "fitness val at parent_id: " << fitnessVect[parent_id] << endl;

    return parentNum;
}


void switchGens(vector<Organism> &currentGen, vector<Organism> &childGen, emp::Systematics<Organism, int> &sys){
    currentGen.swap(childGen);
    childGen.clear();
    sys.Update();
}

bool writeToFile(string filename, int field_one);


int main() {
    emp::Random randNum;

    //emp::Random randMut;

    function<int(Organism)> taxonFunc = [](Organism org){return org.genotype;};

    //function<int(int)> square = [](int squaredNum){return (squaredNum*squaredNum);};

    emp::Systematics<Organism, int> sys(taxonFunc); //optional 3rd arg
    sys.SetTrackSynchronous(true);

    //current gen (vector)
    vector<Organism> currentGen; //begins with currentGen
    //child gen (vector)
    vector<Organism> childGen;

    vector<double> fitnessVect;


    for (int i = 0; i < numOrgs; i++) {
        currentGen.emplace_back(); //currentGen is filled with 10 organism
        sys.AddOrg(currentGen[i], i, systime); //parent is null (removed brackets)
    }

//    for(int i = 0; i < currentGen.size(); i++){
//        cout << currentGen[i] . printVect() << " " << endl;
//    }

    for (int i = 0; i < numGens; i++) {
        cout << "generation: " << i << endl;
        randScope = currentGen.size(); //this tells the chooseOrg function how large the vector is
        //assert(currentGen.size() == 10);

        //calcFitness(currentGen, fitnessVect,randNum);


        for(int r = 0; r < numOrgs; r++){

            chooseOrg(currentGen, randNum);
            //chooseOrgDiversity(fitnessVect, randNum); //chooses the parent of the next generation
            //cout << "parent: " << parentNum << endl;
            sys.SetNextParent(parentNum);
            //currentGen[parentNum].reproduce(childGen, sys); //fills childGen with 10 Organisms

            childGen.emplace_back(currentGen[parentNum].genotype); //fills childGen vector with Organisms
            childGen[r].MutateGenotype(randNum);

            emp::WorldPosition pos(r, 1);
            sys.AddOrg(childGen[r], pos, systime); //removed brackets childGen[i], {i, 0}
            //cout << "size of child population: " << size(childGen) << endl;
        }
        //sys.PrintStatus();
        cout << "phylogenetic diversity: " << sys.GetPhylogeneticDiversity() << endl;

        if(i == TenGens - 1){
            //sys.FindPhyloData();
            cout << "Ten Gens = " << TenGens << endl;
            sys.FindPhyloMultipleGens(TenGens);
            //writeToFile("ChooseOrgDiversityGenotype1000.csv", sys.GetPhylogeneticDiversity())
            TenGens = TenGens + 10;
        }

        for(int j = 0; j < currentGen.size(); j++){
            sys.RemoveOrg(j, systime);
        }


        switchGens(currentGen,childGen, sys); //puts contents of child vector into current vector and deletes content of child vector
        systime++;
    }

    int total_orgs = numGens * numOrgs;

    cout << "generations: " << numGens << " / total organisms: " << total_orgs << endl;
};

bool writeToFile(string filename, int field_one){
    ofstream file;
    file.open(filename, ios_base::app);
    file << field_one << ",";
    file.close();

    return true;
}

//use only end phylo num

//use python script to convert each value to percentile
//order smallest to larget -- '

//end up with file with 100 nums --star num and second num that starts percentile
//in systematics class, we will want to modify some tree calcualtions

//modify phpy diversity and others
//give it optional arg, string pointing to file with percentiles
//file as arg, should open file and read everything

//check out file.h class (tools) -- use to read in file
//at end of phylo funct, if data is providec to normalize, normalize

//figure out which percentile it falls into
