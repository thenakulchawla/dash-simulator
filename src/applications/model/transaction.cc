#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/address.h"
#include "ns3/log.h"
#include "transaction.h"


namespace ns3 {

/**
 *
 * Class Transaction functions
 *
 */

Transaction::Transaction(double transactionSizeBytes, int transactionHeight) : m_transactionSizeBytes(transactionSizeBytes), m_transactionHeight(transactionHeight) {}

Transaction::Transaction() = default;

Transaction::Transaction (const Transaction &transactionSource)
{
  m_transactionSizeBytes = transactionSource.m_transactionSizeBytes;
  m_transactionHeight = transactionSource.m_transactionHeight;
}

Transaction::~Transaction (void)
{
}

double
Transaction::GetTransactionSizeBytes (void) const
{
  return m_transactionSizeBytes;
}

void
Transaction::SetTransactionSizeBytes (double transactionSizeBytes)
{
  m_transactionSizeBytes = transactionSizeBytes;
}

int
Transaction::GetTransactionHeight (void) const
{
  return m_transactionHeight;
}

  void
Transaction::SetTransactionHeight (int transactionHeight)
{
  m_transactionHeight = transactionHeight;
}

} //namespace ns3

