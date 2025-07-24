#include<iostream>
#include "hash_final.h"

using namespace std;

/* Print usage menu */

void menu()
{
    cout<<"--------------------"<<endl;
    cout<<"Enter queries in the following format :"<<endl;
    cout<<"insert <key> <value>     (key: integer, value: string)"<<endl;
    cout<<"delete <key> <mode>      (mode: 0-Lazy / 1-Merge empty buckets / 2-Merge buckets and shrink )"<<endl;
    cout<<"update <key> <new value>"<<endl;
    cout<<"search <key>"<<endl;
    cout<<"display"<<endl;
    cout<<"exit"<<endl;
    cout<<"--------------------"<<endl;
}


int main()
{
    bool show_messages, show_duplicate_buckets;
    int bucket_size, initial_global_depth;
    int key, mode;
    string choice, value;

    // Set show_messages to 0 when taking input using file redirection
    show_messages = 1;

    // Set show_duplicate_buckets to 1 to see all pointers instead of unique ones
    show_duplicate_buckets = 0;

    if(show_messages) { cout<<"Bucket size : "; }
    cin>>bucket_size;
    if(show_messages) { cout<<"Initial global depth : "; }
    cin>>initial_global_depth;

    Directory d(initial_global_depth,bucket_size);
    cout<<endl<<"Initialized directory structure"<<endl;

    if(show_messages)
        menu();

    do
    {
        cout<<endl;
        if(show_messages) { cout<<">>> "; }
        cin>>choice;
        if(choice=="insert")
        {
            cin>>key>>value;
            if(show_messages) { cout<<endl; }
            d.insert(key,value,0);
        }
        else if(choice=="delete")
        {
            cin>>key>>mode;
            if(show_messages) { cout<<endl; }
            d.remove(key,mode);
        }
        else if(choice=="update")
        {
            cin>>key>>value;
            if(show_messages) { cout<<endl; }
            d.update(key,value);
        }
        else if(choice=="search")
        {
            cin>>key;
            if(show_messages) { cout<<endl; }
            d.search(key);
        }
        else if(choice=="display")
        {
            if(show_messages) { cout<<endl; }
            d.display(show_duplicate_buckets);
        }
        else if(choice=="save")
        {
            string filename;
            cin>>filename;
            d.saveToFile(filename);
            cout<<"Directory saved to "<<filename<<endl;
        }
    } while(choice!="exit");

    return 0;
}

