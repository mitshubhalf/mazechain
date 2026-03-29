#ifndef TRANSACTION_H
#define TRANSACTION_H
#include <string>

struct Transaction {
 std::string from;
 std::string to;
 int amount;
 std::string signature;

 std::string toString() const {
  return from+to+std::to_string(amount);
 }
};

#endif
