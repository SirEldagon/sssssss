#include <iostream>
#include <vector>
#include <future>
#include <algorithm>
#include <atomic>

// Максимальное число одновременно запущенных потоков
const unsigned MAX_THREADS = std::thread::hardware_concurrency(); // или задайте вручную

// Атомарный счетчик активных потоков
std::atomic<unsigned> active_threads(0);

// Функция для слияния двух отсортированных частей
void merge(std::vector<int>& arr, size_t left, size_t mid, size_t right) {
    std::vector<int> temp(right - left + 1);
    size_t i = left, j = mid + 1, k = 0;

    while (i <= mid && j <= right) {
        if (arr[i] <= arr[j]) {
            temp[k++] = arr[i++];
        } else {
            temp[k++] = arr[j++];
        }
    }

    while (i <= mid) {
        temp[k++] = arr[i++];
    }

    while (j <= right) {
        temp[k++] = arr[j++];
    }

    std::copy(temp.begin(), temp.end(), arr.begin() + left);
}

// Многопоточная реализация merge sort
std::future<void> parallel_merge_sort(std::vector<int>& arr, size_t left, size_t right) {
    return std::async(std::launch::async, [&arr, left, right]() {
        if (left >= right) return;

        size_t mid = left + (right - left) / 2;

        // Решение о запуске новых потоков
        std::future<void> left_future, right_future;

        // Проверяем, можем ли запустить новые потоки
        if (active_threads < MAX_THREADS) {
            active_threads += 2; // увеличиваем счетчик активных потоков
            left_future = parallel_merge_sort(arr, left, mid);
            right_future = parallel_merge_sort(arr, mid + 1, right);
        } else {
            // Если потоков больше не запускаем, делаем рекурсивно в текущем потоке
            left_future = std::async(std::launch::deferred, [&arr, left, mid]() {
                parallel_merge_sort(arr, left, mid).get();
            });
            right_future = std::async(std::launch::deferred, [&arr, mid, right]() {
                parallel_merge_sort(arr, mid + 1, right).get();
            });
        }

        // Ожидаем завершения сортировки обеих частей
        left_future.get();
        right_future.get();

        // После завершения сортировки обеих частей, сливаем их
        merge(arr, left, mid, right);

        // Уменьшаем счетчик активных потоков
        active_threads -= 2;
    });
}

int main() {
    // Пример использования
    std::vector<int> data = {38, 27, 43, 3, 9, 82, 10, 15, 7, 20};

    std::cout << "Исходный массив:\n";
    for (auto num : data) std::cout << num << " ";
    std::cout << "\n";

    // Запуск многопоточного merge sort
    auto future = parallel_merge_sort(data, 0, data.size() - 1);
    future.get(); // ждём завершения сортировки

    std::cout << "Отсортированный массив:\n";
    for (auto num : data) std::cout << num << " ";
    std::cout << "\n";

    return 0;
}
