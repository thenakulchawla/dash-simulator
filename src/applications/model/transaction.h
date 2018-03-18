#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <vector>
#include <map>
#include "ns3/address.h"
#include <string>
#include <sstream>
#include <random>
#include <unordered_map>
#include "ns3/address.h"
#include "ns3/address-utils.h"


namespace ns3 {

class Transaction
{
public:
Transaction (double transactionSizeBytes,  std::string transactionHash, std::string transactionShortHash);
Transaction ();
Transaction (const Transaction &transactionSource); //copy constructor
virtual ~Transaction (void);

std::string GetTransactionHash (void) const;
void SetTransactionHash (std::string transactionHash);

std::string GetTransactionShortHash (void) const;
void SetTransactionShortHash (std::string transactionShortHash);

double GetTransactionSizeBytes (void) const;
void SetTransactionSizeBytes (double transactionSizeBytes);

std::string sha256(const std::string data);

// std::vector<Transaction> GetMempoolTransactions (void) const;

// Ipv4Address GetCreatedAtIpv4 (void) const;
// void SetCreatedAtIpv4 (Ipv4Address createdAtIpv4);

protected:
double m_transactionSizeBytes;
std::string m_transactionHash, m_transactionShortHash;

};

class Mempool
{
public: 

Mempool(void);
virtual ~Mempool (void);

void AddTransaction(const Transaction& newTransaction);
std::unordered_map<std::string,Transaction> GetMempoolTransactions (void) const;
int GetMempoolSize (void) const;
bool HasShortTransaction (std::string shortHash);
Transaction GetTransactionWithShortHash (std::string shortHash);
void DeleteTransactionWithShortHash(std::string shortHash);

protected:
// std::vector<Transaction> m_transactions; //vector that contains all the transactions of the mempool
std::unordered_map<std::string, Transaction> m_transactions; // replacing vector with map 
int m_totalTransactions;

};

}  //namespace ns3

#endif /*Transaction_H*/
												
