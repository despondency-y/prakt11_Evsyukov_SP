// prakt11_Evsyukov_SP.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <cstdlib> 
#include <ctime>  


using namespace std;


struct Robot {
    int Id;
    int Energy = 60;
    int Score = 0;
    string Status = "работает";
    HANDLE hThread = NULL;
};




int* trials = nullptr;       
int trials_count = 0;        
Robot* robots[10];         
HANDLE thread_handles[10];   
int robot_count = 0;         
mutex console_mtx;           

void Message(const string& msg) {
    lock_guard<mutex> lock(console_mtx);
    cout << msg << endl;
}


DWORD WINAPI RobotWorker(LPVOID lpParam) {
    Robot* robot = reinterpret_cast<Robot*>(lpParam);


    srand(time(0) + robot->Id);

    Message("--- Робот #" + to_string(robot->Id) + " начал испытания.");


    for (int i = 0; i < trials_count; i++) {
        int code = trials[i];


        int rand_delay = (rand() % 3) + 1;
        Sleep(rand_delay * 1000);

        int duration = 0;
        string name = "";
        int energy_cost = 0;
        int score_reward = 0;

        switch (code) {
        case 1:
            name = "Препятствие"; duration = 3; energy_cost = 10; score_reward = 15;
            break;
        case 2:
            name = "Спринт"; duration = 4; energy_cost = 20; score_reward = 30;
            break;
        case 3:

            duration = (rand() % 5) + 1;
            name = "Подзарядка";
            energy_cost = -(duration * 10);
            score_reward = 0;
            break;
        case 4:
            name = "Марафон"; duration = 10; energy_cost = 60; score_reward = 55;
            break;
        }

        if (code != 3 && robot->Energy < energy_cost) {
            Message("Робот #" + to_string(robot->Id) + ": Пропустил [" + name + "] (мало энергии).");
            continue;
        }

        Message("[ ] Робот #" + to_string(robot->Id) + ": Выполняет [" + name + "] (" + to_string(duration) + " сек)...");
        Sleep(duration * 1000);

        if (code == 3) {
            robot->Energy = min(robot->Energy - energy_cost, 100);
        }
        else {
            robot->Energy -= energy_cost;
            robot->Score += score_reward;
        }

        Message("Робот #" + to_string(robot->Id) + ": Завершил [" + name + "]. Очки: " + to_string(robot->Score) + ", Энергия: " + to_string(robot->Energy));
    }

    robot->Status = "завершил испытания";
    Message("\tРобот #" + to_string(robot->Id) + " ПОЛНОСТЬЮ ФИНИШИРОВАЛ.");
    return 0;
}

int main()
{
    setlocale(LC_ALL, "Russian");

    srand(time(0));

    cout << "Введите количество роботов (от 3 до 10): ";
    cin >> robot_count;

    if (robot_count < 3 || robot_count > 10) {
        cout << "--- Неверное количество роботов" << endl;
        return 1;
    }

    ifstream in("arena.dat", ios::binary | ios::ate);
    if (!in) {
        cout << "--- Ошибка: файл arena.dat не найден" << endl;
        return 1;
    }

    long long file_size = in.tellg();
    trials_count = file_size / sizeof(int);

    trials = new int[trials_count];

    in.seekg(0, ios::beg);
    in.read(reinterpret_cast<char*>(trials), file_size);
    in.close();

    for (int i = 0; i < robot_count; ++i) {
        Robot* r = new Robot();
        r->Id = i + 1;

        // r->Energy = 10 + (i * 30); 

        robots[i] = r;

        r->hThread = CreateThread(NULL, 0, RobotWorker, r, 0, NULL);
        if (r->hThread != NULL) {
            thread_handles[i] = r->hThread;
        }
    }

    Sleep(100);

    cout << "\n--- ТУРНИР НАЧАЛСЯ ---" << endl;
    cout << "Управление:\n  p [ID] - Пауза\n  r [ID] - Старт\n  s - Статус всех\n  e - Итоги турнира\n" << endl;

    string cmd;
    while (true) {
        cin >> cmd;
        if (cmd == "p") {
            int id; cin >> id;
            if (id > 0 && id <= robot_count) {
                SuspendThread(robots[id - 1]->hThread);
                robots[id - 1]->Status = "приостановлен";
                Message("Оператор приостановил Робота #" + to_string(id));
            }
        }
        else if (cmd == "r") {
            int id; cin >> id;
            if (id > 0 && id <= robot_count) {
                ResumeThread(robots[id - 1]->hThread);
                robots[id - 1]->Status = "работает";
                Message("Оператор возобновил Робота #" + to_string(id));
            }
        }
        else if (cmd == "s") {
            lock_guard<mutex> lock(console_mtx);
            cout << "\n\t--- СТАТУС РОБОТОВ ---" << endl;
            for (int i = 0; i < robot_count; i++) {
                cout << "Робот #" << robots[i]->Id << " | Очки: " << robots[i]->Score
                    << " | Энергия: " << robots[i]->Energy << " | Статус: " << robots[i]->Status << endl;
            }
            cout << "--------------\n" << endl;
        }
        else if (cmd == "e") {
            break;
        }
    }

    WaitForMultipleObjects(robot_count, thread_handles, TRUE, INFINITE);

    for (int i = 0; i < robot_count - 1; i++) {
        for (int j = 0; j < robot_count - i - 1; j++) {
            if (robots[j]->Score < robots[j + 1]->Score) {
                Robot* temp = robots[j];
                robots[j] = robots[j + 1];
                robots[j + 1] = temp;
            }
        }
    }

    cout << "\n\t--- ФИНАЛЬНЫЕ РЕЗУЛЬТАТЫ ---" << endl;
    for (int i = 0; i < robot_count; i++) {
        cout << "Робот #" << robots[i]->Id << " — Очки: " << robots[i]->Score << " (Энергия: " << robots[i]->Energy << ")" << endl;
    }

    int max_score = robots[0]->Score;
    int winners_count = 0;
    for (int i = 0; i < robot_count; i++) {
        if (robots[i]->Score == max_score) winners_count++;
    }

    if (winners_count >= 2) {
        cout << "\nРезультат турнира: ничья" << endl;
    }
    else {
        cout << "\n\t--- ПОБЕДИТЕЛЬ ТУРНИРА: Робот #" << robots[0]->Id << " с результатом " << max_score << " очков" << endl;
    }


    for (int i = 0; i < robot_count; i++) {
        CloseHandle(robots[i]->hThread);
        delete robots[i];
    }
    delete[] trials; 

    return 0;
}

