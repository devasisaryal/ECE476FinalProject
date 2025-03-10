#include "dsred-queue-disc.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DsRedQueueDisc");
NS_OBJECT_ENSURE_REGISTERED (DsRedQueueDisc);

TypeId
DsRedQueueDisc::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::DsRedQueueDisc")
    .SetParent<RedQueueDisc> ()
    .AddConstructor<DsRedQueueDisc> ()
    .AddAttribute ("MidThreshold", "Middle threshold for double slope RED",
                  DoubleValue (30), // Example default
                  MakeDoubleAccessor (&DsRedQueueDisc::m_midThreshold),
                  MakeDoubleChecker<double> ())
    .AddAttribute ("Gamma", "Scaling factor for slope calculation",
                  DoubleValue (1.0),
                  MakeDoubleAccessor (&DsRedQueueDisc::m_gamma),
                  MakeDoubleChecker<double> ());
  return tid;
}

DsRedQueueDisc::DsRedQueueDisc () : m_midThreshold (30), m_gamma (1.0) {}

DsRedQueueDisc::~DsRedQueueDisc () {}

void
DsRedQueueDisc::SetMidThreshold (double mid)
{
  m_midThreshold = mid;
}

void
DsRedQueueDisc::SetGamma (double gamma)
{
  m_gamma = gamma;
}

double
DsRedQueueDisc::CalculatePNew (void)
{
  double avg = m_qAvg;
  double minTh = m_minTh;
  double maxTh = m_maxTh;
  double pMax = m_lInterm;

  double alpha = (pMax-m_gamma)/(m_midThreshold-minTh);
  double beta = m_gamma/(maxTh-m_midThreshold);

  if (avg < minTh)
    {
      return 0.0;
    }
  else if (avg < m_midThreshold)
    {
      return alpha * (avg - minTh);
    }
  else if (avg < maxTh)
    {
        return 1 - m_gamma + beta * (avg - m_midThreshold);
    }
  else
    {
      return 1.0; // Drop all packets above max threshold
    }
}

} // namespace ns3
