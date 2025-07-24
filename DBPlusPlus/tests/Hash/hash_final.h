#pragma once    
//#include <bits/stdc++.h>
#include <map>
#include <vector>
#include <iostream>
#include <string>
#include <set>
#include <fstream>
using namespace std;


class Bucket {
        int depth,size;
        std::map<int, vector<string>> values;
    public:
        Bucket(int depth, int size);
        int insert(int key,string value);
        int remove(int key);
        int update(int key, string value);
        void search(int key);
        int isFull(void);
        int isEmpty(void);
        int getDepth(void);
        int increaseDepth(void);
        int decreaseDepth(void);
        std::map<int, vector<string>> copy(void);
        void clear(void);
        void display(void);
};



Bucket::Bucket(int depth, int size){
    this->depth = depth;
    this->size = size;
}

int Bucket::insert(int key, string value){
    std::map<int,vector<string>>::iterator it;
    it = values.find(key);
    if(it!=values.end()){
        it->second.push_back(value); //adds new value
        return 2;
    }
    if(isFull()){
        return 0;
    }
    values[key] = std::vector<string>{value};
    return 1;
}

int Bucket::remove(int key){
    std::map<int,vector<string>>::iterator it;
    it = values.find(key);
    if(it!=values.end())    {
        values.erase(it);
        return 1;
    }
    else
    {
        cout<<"Cannot remove : This key does not exists"<<endl;
        return 0;
    }
}

int Bucket::update(int key, string value){    
    std::map<int,vector<string>>::iterator it;
    it = values.find(key);
    if(it!=values.end())
    {
        values[key] = std::vector<string>{value};
        cout<<"Value updated"<<endl;
        return 1;
    }
    else
    {
        cout<<"Cannot update : This key does not exists"<<endl;
        return 0;
    }
}

void Bucket::search(int key){
    std::map<int,vector<string>>::iterator it;
    it = values.find(key);
    if(it!=values.end())    {
        if (!it->second.empty()){
            cout<<"Values = ";
            for(int i=0;i<it->second.size();i++){
                cout<<it->second[i]<<" | ";
            }
        }
        else{
            cout<<"Value is empty for this key"<<endl;
        }
    }
    else{
        cout<<"This key does not exists"<<endl;
    }
}

int Bucket::isFull(void){
    if(values.size()==size)
        return 1;
    else
        return 0;
}

int Bucket::isEmpty(void){
    if(values.size()==0)
        return 1;
    else
        return 0;
}

int Bucket::getDepth(void){
    return depth;
}

int Bucket::increaseDepth(void){
    depth++;
    return depth;
}

int Bucket::decreaseDepth(void){
    depth--;
    return depth;
}

std::map<int, vector<string>> Bucket::copy(void){
    std::map<int, std::vector<std::string>> temp;

    for (const auto& entry : values) {
        const int key = entry.first;
        const auto& vec = entry.second;
        // Sólo copiamos si el vector no está vacío
        if (!vec.empty()) {
            temp.emplace(key, vec);
        }
    }
    return temp;
}

void Bucket::clear(void){
    values.clear();
}

void Bucket::display(){
    std::map<int, std::vector<string>>::iterator it;
    for(it=values.begin(); it != values.end(); it++)
        cout<<it->first<<" ";
    cout<<endl;
}

class Directory {
        int global_depth, bucket_size;
        std::vector<Bucket*> buckets;
        int hash(int n);
        int pairIndex(int bucket_no, int depth);
        void grow(void);
        void shrink(void);
        void split(int bucket_no);
        void merge(int bucket_no);
        string bucket_id(int n);
    public:
        Directory(int depth, int bucket_size);
        void insert(int key,string value,bool reinserted);
        void remove(int key,int mode);
        void update(int key, string value);
        void search(int key);
        void display(bool duplicates);

        void saveToFile(const std::string& filename);
};

Directory::Directory(int depth, int bucket_size){
    this->global_depth = depth;
    this->bucket_size = bucket_size;
    for(int i = 0 ; i < 1<<depth ; i++ )
    {
        buckets.push_back(new Bucket(depth,bucket_size));
    }
}

int Directory::hash(int n){
    size_t h = std::hash<int>{}(n);
    return static_cast<int>(h & ((1ULL<<global_depth)-1));
}

int Directory::pairIndex(int bucket_no, int depth){
    return bucket_no^(1<<(depth-1));
}

void Directory::grow(void){    
    for(int i = 0 ; i < 1<<global_depth ; i++ )
        buckets.push_back(buckets[i]);
    global_depth++;
}

void Directory::shrink(void){
    int i,flag=1;
    for( i=0 ; i<buckets.size() ; i++ )
    {
        if(buckets[i]->getDepth()==global_depth)
        {
            flag=0;
            return;
        }
    }
    global_depth--;
    for(i = 0 ; i < 1<<global_depth ; i++ )
        buckets.pop_back();
}

void Directory::split(int bucket_no){
    int local_depth,pair_index,index_diff,dir_size,i;
    map<int, vector<string>> temp;
    map<int, vector<string>>::iterator it;

    local_depth = buckets[bucket_no]->increaseDepth();
    if(local_depth>global_depth)
        grow();
    pair_index = pairIndex(bucket_no,local_depth);
    buckets[pair_index] = new Bucket(local_depth,bucket_size);
    temp = buckets[bucket_no]->copy();
    buckets[bucket_no]->clear();
    index_diff = 1<<local_depth;
    dir_size = 1<<global_depth;
    for( i=pair_index-index_diff ; i>=0 ; i-=index_diff )
        buckets[i] = buckets[pair_index];
    for( i=pair_index+index_diff ; i<dir_size ; i+=index_diff )
        buckets[i] = buckets[pair_index];

    for (auto& entry : temp) {
        int key = entry.first;
        for (auto& val : entry.second) {
            // aquí reinsertamos cada string, marcándolo como reinsertado
            insert(key, val, /*reinserted=*/true);
        }
    }
}

void Directory::merge(int bucket_no){
    int local_depth,pair_index,index_diff,dir_size,i;

    local_depth = buckets[bucket_no]->getDepth();
    pair_index = pairIndex(bucket_no,local_depth);
    index_diff = 1<<local_depth;
    dir_size = 1<<global_depth;

    if( buckets[pair_index]->getDepth() == local_depth )
    {
        buckets[pair_index]->decreaseDepth();
        delete(buckets[bucket_no]);
        buckets[bucket_no] = buckets[pair_index];
        for( i=bucket_no-index_diff ; i>=0 ; i-=index_diff )
            buckets[i] = buckets[pair_index];
        for( i=bucket_no+index_diff ; i<dir_size ; i+=index_diff )
            buckets[i] = buckets[pair_index];
    }
}

string Directory::bucket_id(int n){
    int d;
    string s;
    d = buckets[n]->getDepth();
    s = "";
    while(n>0 && d>0)
    {
        s = (n%2==0?"0":"1")+s;
        n/=2;
        d--;
    }
    while(d>0)
    {
        s = "0"+s;
        d--;
    }
    return s;
}

void Directory::insert(int key,string value,bool reinserted){
    int bucket_no = hash(key);
    int status = buckets[bucket_no]->insert(key,value);
    if(status==1)
    {
        if(!reinserted)
            cout<<"Inserted key "<<key<<" in bucket "<<bucket_id(bucket_no)<<endl;
        else
            cout<<"Moved key "<<key<<" to bucket "<<bucket_id(bucket_no)<<endl;
    }
    else if(status==0)
    {
        split(bucket_no);
        insert(key,value,reinserted);
    }
    else
    {
        cout<<"Key "<<key<<" already exists in bucket "<<bucket_id(bucket_no)<<"added value "<<endl;
    }
}

void Directory::remove(int key,int mode){
    int bucket_no = hash(key);
    if(buckets[bucket_no]->remove(key))
        cout<<"Deleted key "<<key<<" from bucket "<<bucket_id(bucket_no)<<endl;
    if(mode>0)
    {
        if(buckets[bucket_no]->isEmpty() && buckets[bucket_no]->getDepth()>1)
            merge(bucket_no);
    }
    if(mode>1)
    {
        shrink();
    }
}

void Directory::update(int key, string value){
    int bucket_no = hash(key);
    buckets[bucket_no]->update(key,value);
}

void Directory::search(int key){
    int bucket_no = hash(key);
    cout<<"Searching key "<<key<<" in bucket "<<bucket_id(bucket_no)<<endl;
    buckets[bucket_no]->search(key);
}

void Directory::display(bool duplicates){
    int i,j,d;
    string s;
    std::set<string> shown;
    cout<<"Global depth : "<<global_depth<<endl;
    for(i=0;i<buckets.size();i++)
    {
        d = buckets[i]->getDepth();
        s = bucket_id(i);
        if(duplicates || shown.find(s)==shown.end())
        {
            shown.insert(s);
            for(j=d;j<=global_depth;j++)
                cout<<" ";
            cout<<s<<" => ";
            buckets[i]->display();
        }
    }
}

void Directory::saveToFile(const std::string& filename) {
    string filename=filename+".txt"
    std::ofstream ofs(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error abriendo archivo: " << filename << "\n";
        return;
    }

    // Magic header y parámetros
    ofs << "EXT_HASH_V1\n";
    ofs << "BUCKET_SIZE: " << bucket_size << "\n";
    ofs << "GLOBAL_DEPTH: " << global_depth << "\n\n";

    // 1) Sección Directorio
    ofs << "# Directory\n";
    int dirSize = 1 << global_depth;
    // Asignamos etiquetas únicas a cada Bucket*
    std::map<Bucket*, std::string> labels;
    int counter = 0;
    for (int i = 0; i < dirSize; ++i) {
        Bucket* b = buckets[i];
        if (labels.find(b) == labels.end()) {
            labels[b] = "B" + std::to_string(counter++);
        }
        ofs << bucket_id(i) << " -> " << labels[b] << "\n";
    }

    // 2) Sección Buckets
    ofs << "\n# Buckets\n";
    for (auto& p : labels) {
        Bucket* b = p.first;
        const std::string& label = p.second;
        ofs << label 
            << " (depth=" << b->getDepth() << "): ";

        // Recuperamos sólo las entradas no vacías
        auto content = b->copy();
        bool firstKV = true;
        for (auto& kv : content) {
            if (!firstKV) ofs << "; ";
            firstKV = false;

            // clave
            ofs << kv.first << "=";

            // lista de valores separados por coma
            for (size_t i = 0; i < kv.second.size(); ++i) {
                ofs << kv.second[i];
                if (i + 1 < kv.second.size()) 
                    ofs << ",";
            }
        }
        ofs << "\n";
    }

    ofs.close();
}