#ifndef CATCH_CONFIG_MAIN
#define CATCH_CONFIG_MAIN
#endif

#include "atm.hpp"
#include "catch.hpp"
#include <fstream>
#include <string>
#include <vector>

bool CompareFiles(const std::string& p1, const std::string& p2) {
  std::ifstream f1(p1);
  std::ifstream f2(p2);

  if (f1.fail() || f2.fail()) {
    return false;
  }

  std::string f1_read;
  std::string f2_read;
  while (f1.good() || f2.good()) {
    f1 >> f1_read;
    f2 >> f2_read;
    if (f1_read != f2_read || (f1.good() && !f2.good()) ||
        (!f1.good() && f2.good())) {
      return false;
    }
  }
  return true;
}

TEST_CASE("RegisterAccount creates account and empty transaction list", "[register]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);

  auto& accounts = atm.GetAccounts();
  auto& transactions = atm.GetTransactions();

  REQUIRE(accounts.contains({12345678, 1234}));
  REQUIRE(transactions.contains({12345678, 1234}));
  REQUIRE(accounts.at({12345678, 1234}).owner_name == "Sam Sepiol");
  REQUIRE(accounts.at({12345678, 1234}).balance == Approx(300.30));
  REQUIRE(transactions.at({12345678, 1234}).empty());
}

TEST_CASE("RegisterAccount throws on duplicate exact card and pin", "[register]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);

  REQUIRE_THROWS_AS(
      atm.RegisterAccount(12345678, 1234, "Other Name", 999.99),
      std::invalid_argument);
}

TEST_CASE("RegisterAccount allows same card with different pin", "[register]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);
  atm.RegisterAccount(12345678, 9999, "Sam Two", 500.00);

  auto& accounts = atm.GetAccounts();
  REQUIRE(accounts.contains({12345678, 1234}));
  REQUIRE(accounts.contains({12345678, 9999}));
  REQUIRE(accounts.size() == 2);
}

TEST_CASE("WithdrawCash subtracts balance and records transaction", "[withdraw]") {
  Atm atm;
  atm.RegisterAccount(11111111, 1111, "A", 300.00);

  std::size_t before = atm.GetTransactions().at({11111111, 1111}).size();
  atm.WithdrawCash(11111111, 1111, 20.00);

  REQUIRE(atm.CheckBalance(11111111, 1111) == Approx(280.00));
  REQUIRE(atm.GetTransactions().at({11111111, 1111}).size() == before + 1);
}

TEST_CASE("WithdrawCash throws for missing account", "[withdraw]") {
  Atm atm;

  REQUIRE_THROWS_AS(
      atm.WithdrawCash(11111111, 1111, 20.00),
      std::invalid_argument);
}

TEST_CASE("WithdrawCash throws invalid_argument on negative amount", "[withdraw]") {
  Atm atm;
  atm.RegisterAccount(11111111, 1111, "A", 300.00);

  double before_balance = atm.CheckBalance(11111111, 1111);
  std::size_t before_count = atm.GetTransactions().at({11111111, 1111}).size();

  REQUIRE_THROWS_AS(
      atm.WithdrawCash(11111111, 1111, -20.00),
      std::invalid_argument);

  REQUIRE(atm.CheckBalance(11111111, 1111) == Approx(before_balance));
  REQUIRE(atm.GetTransactions().at({11111111, 1111}).size() == before_count);
}

TEST_CASE("WithdrawCash throws runtime_error on overdraft", "[withdraw]") {
  Atm atm;
  atm.RegisterAccount(11111111, 1111, "A", 100.00);

  double before_balance = atm.CheckBalance(11111111, 1111);
  std::size_t before_count = atm.GetTransactions().at({11111111, 1111}).size();

  REQUIRE_THROWS_AS(
      atm.WithdrawCash(11111111, 1111, 100.01),
      std::runtime_error);

  REQUIRE(atm.CheckBalance(11111111, 1111) == Approx(before_balance));
  REQUIRE(atm.GetTransactions().at({11111111, 1111}).size() == before_count);
}

TEST_CASE("WithdrawCash allows withdrawing entire balance", "[withdraw]") {
  Atm atm;
  atm.RegisterAccount(11111111, 1111, "A", 100.00);

  atm.WithdrawCash(11111111, 1111, 100.00);

  REQUIRE(atm.CheckBalance(11111111, 1111) == Approx(0.00));
}

TEST_CASE("DepositCash adds balance and records transaction", "[deposit]") {
  Atm atm;
  atm.RegisterAccount(22222222, 2222, "B", 100.00);

  std::size_t before = atm.GetTransactions().at({22222222, 2222}).size();
  atm.DepositCash(22222222, 2222, 25.50);

  REQUIRE(atm.CheckBalance(22222222, 2222) == Approx(125.50));
  REQUIRE(atm.GetTransactions().at({22222222, 2222}).size() == before + 1);
}

TEST_CASE("DepositCash throws for missing account", "[deposit]") {
  Atm atm;

  REQUIRE_THROWS_AS(
      atm.DepositCash(22222222, 2222, 25.50),
      std::invalid_argument);
}

TEST_CASE("DepositCash throws invalid_argument on negative amount", "[deposit]") {
  Atm atm;
  atm.RegisterAccount(22222222, 2222, "B", 100.00);

  double before_balance = atm.CheckBalance(22222222, 2222);
  std::size_t before_count = atm.GetTransactions().at({22222222, 2222}).size();

  REQUIRE_THROWS_AS(
      atm.DepositCash(22222222, 2222, -25.50),
      std::invalid_argument);

  REQUIRE(atm.CheckBalance(22222222, 2222) == Approx(before_balance));
  REQUIRE(atm.GetTransactions().at({22222222, 2222}).size() == before_count);
}

TEST_CASE("PrintLedger throws for missing account", "[ledger]") {
  Atm atm;

  REQUIRE_THROWS_AS(
      atm.PrintLedger("./missing.txt", 99999999, 9999),
      std::invalid_argument);
}

TEST_CASE("PrintLedger prints expected ledger content", "[ledger]") {
  Atm atm;
  atm.RegisterAccount(12345678, 1234, "Sam Sepiol", 300.30);

  auto& transactions = atm.GetTransactions();
  transactions[{12345678, 1234}].push_back(
      "Withdrawal - Amount: $200.40, Updated Balance: $99.90");
  transactions[{12345678, 1234}].push_back(
      "Deposit - Amount: $40000.00, Updated Balance: $40099.90");
  transactions[{12345678, 1234}].push_back(
      "Deposit - Amount: $32000.00, Updated Balance: $72099.90");

  atm.PrintLedger("./prompt.txt", 12345678, 1234);
  REQUIRE(CompareFiles("./ex-1.txt", "./prompt.txt"));
}