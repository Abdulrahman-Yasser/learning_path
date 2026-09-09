#include <condition_variable>
#include <iostream>
#include <mutex>
#include <thread>

int balance = 0;
std::mutex m;
std::condition_variable cv;

void addMoney(int amount) {
	{
		std::lock_guard<std::mutex> lock(m);
		balance += amount;
		std::cout << "Added $" << amount << ". Balance: $" << balance << '\n';
	}

	cv.notify_one();
}

void withdrawMoney(int amount) {
	std::unique_lock<std::mutex> lock(m);

	cv.wait(lock, [amount] {
		return balance >= amount;
	});

	balance -= amount;
	std::cout << "Withdrew $" << amount << ". Balance: $" << balance << '\n';
}

int main() {
	std::thread add_thread(addMoney, 100);
	std::thread withdraw_thread(withdrawMoney, 60);

	add_thread.join();
	withdraw_thread.join();
}
