#include <stdlib.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <stdio.h>
#include <condition_variable>
using namespace std;


class Cell{
    private:
        std::mutex my_mutex_;
        std::condition_variable cv_;
        bool has_value = false;
        int value = 0;
    public:
        void put(int value);
        void put();
        int take();
};


void Cell::put(int value){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return !has_value; });
    this->value = value;
    has_value = true;
    this->cv_.notify_one();
}
void Cell::put(){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return !has_value; });
    has_value = true;
    this->cv_.notify_one();
}


int Cell::take(){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return has_value; });
    has_value = false;
    return this->value;
}



void thread_1(Cell& v) { 
    for (int i = 0; i < 100000; i++){
        int v_t = v.take();
        v.put(v_t+1);
    }
    cout << "Last value th1 is " << v.take() << endl;
    v.put();
}
void thread_2(Cell& v) { 
    for (int i = 0; i < 100000; i++){
        int v_t = v.take();
        v.put(v_t+1);
    }
    cout << "Last value th2 is " << v.take() << endl;
    v.put();
}

int main(){
    Cell my_val;
    my_val.put(0);
    std::thread th_1(thread_1, std::ref(my_val)), th_2(thread_2, std::ref(my_val));
    th_1.join();
    th_2.join();
    // while(1);
}
