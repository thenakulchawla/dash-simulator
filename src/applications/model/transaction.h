
#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <vector>
#include <map>
#include "ns3/address.h"

namespace ns3 {

class Transaction
{
public:
Transaction (double transactionSizeBytes, int transactionHeight );
Transaction ();
Transaction (const Transaction &transactionSource); //copy constructor
virtual ~Transaction (void);

double GetTransactionSizeBytes (void) const;
void SetTransactionSizeBytes (double transactionSizeBytes);

int GetTransactionHeight (void) const;
void SetTransactionHeight (int transactionHeight);

protected:
double m_transactionSizeBytes;
int m_transactionHeight;
};
}  //namespace ns3

#endif /*Transaction_H*/
												
