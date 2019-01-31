/*
#include <iostream>
#include <chrono>
#include <thread>
 
int main()
{
    using namespace std::chrono_literals;
    std::cout << "Hello waiter" << std::endl; // flush is intentional
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(2us);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << "Waited " << elapsed.count() << " ms\n";
}
*/

 // std::chrono::seconds s =std::chrono::duration_cast<std::chrono::seconds> (dtn);

#include <iostream>
#include <chrono>
#include <unistd.h>

int main ()
{

  using namespace std::chrono;

  system_clock::time_point tp,tp2;                // epoch value
  system_clock::duration dtn (duration<int>(1));  // 1 second
  tp=high_resolution_clock::now();
  sleep(1);
  tp2=high_resolution_clock::now();
  dtn=tp2-tp;
  std::chrono::milliseconds s =std::chrono::duration_cast<std::chrono::milliseconds> (dtn);
  std::cout<<s.count()<<std::endl;
  return 0;
}
