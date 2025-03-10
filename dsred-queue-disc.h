#ifndef DSRED_QUEUE_DISC_H
#define DSRED_QUEUE_DISC_H

#include "ns3/red-queue-disc.h"

namespace ns3 {

class DsRedQueueDisc : public RedQueueDisc
{
public:
  static TypeId GetTypeId (void);

  DsRedQueueDisc ();
  virtual ~DsRedQueueDisc();

  void SetMidThreshold (double mid);
  void SetGamma (double gamma);

protected:
  virtual double CalculatePNew (void) override; // Override RED probability function

private:
  double m_midThreshold; // Middle threshold
  double m_gamma;        // Gamma factor
};

} // namespace ns3

#endif /* DSRED_QUEUE_DISC_H */
