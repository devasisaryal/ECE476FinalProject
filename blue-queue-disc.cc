#include "blue-queue-disc.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/drop-tail-queue.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("BlueQueueDisc");

NS_OBJECT_ENSURE_REGISTERED(BlueQueueDisc);

TypeId
BlueQueueDisc::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BlueQueueDisc")
        .SetParent<QueueDisc>()
        .SetGroupName("TrafficControl")
        .AddConstructor<BlueQueueDisc>()
        .AddAttribute("MaxSize",
                      "The maximum number of packets accepted by this queue disc",
                      QueueSizeValue(QueueSize("100p")),
                      MakeQueueSizeAccessor(&QueueDisc::SetMaxSize, &QueueDisc::GetMaxSize),
                      MakeQueueSizeChecker())
        .AddAttribute("Increment",
                      "Increment value for drop probability on queue overflow",
                      DoubleValue(0.0205),
                      MakeDoubleAccessor(&BlueQueueDisc::m_increment),
                      MakeDoubleChecker<double>(0.0, 1.0))
        .AddAttribute("Decrement",
                      "Decrement value for drop probability on queue underflow",
                      DoubleValue(0.00025),
                      MakeDoubleAccessor(&BlueQueueDisc::m_decrement),
                      MakeDoubleChecker<double>(0.0, 1.0))
        .AddAttribute("FreezeTime",
                      "Time interval between drop probability updates",
                      TimeValue(Seconds(0.1)),
                      MakeTimeAccessor(&BlueQueueDisc::m_freezeTime),
                      MakeTimeChecker());

    return tid;
}

BlueQueueDisc::BlueQueueDisc()
    : QueueDisc(QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE)
{
    NS_LOG_FUNCTION(this);
    m_uv = CreateObject<UniformRandomVariable>();
}

BlueQueueDisc::~BlueQueueDisc()
{
    NS_LOG_FUNCTION(this);
}

void
BlueQueueDisc::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_uv = nullptr;
    QueueDisc::DoDispose();
}

int64_t
BlueQueueDisc::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_uv->SetStream(stream);
    return 1;
}

double
BlueQueueDisc::GetDropProbability() const
{
    return m_dropProb;
}

bool
BlueQueueDisc::DoEnqueue(Ptr<QueueDiscItem> item)
{
    NS_LOG_FUNCTION(this << item);

    QueueSize currentSize = GetCurrentSize();
    if (currentSize >= GetMaxSize())
    {
        UpdateDropProb(true);  // Overflow event
        double u = m_uv->GetValue();
        if (u <= m_dropProb)
        {
            NS_LOG_DEBUG("\t Dropping due to probability " << m_dropProb);
            DropBeforeEnqueue(item, PROB_DROP);
            return false;
        }
        NS_LOG_DEBUG("\t Queue full, dropping packet");
        DropBeforeEnqueue(item, FORCED_DROP);
        return false;
    }

    bool retval = GetInternalQueue(0)->Enqueue(item);
    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return retval;
}

void
BlueQueueDisc::InitializeParams()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_INFO("Initializing BLUE params.");

    m_dropProb = 0.0;
    m_lastUpdate = NanoSeconds(0);
}

void
BlueQueueDisc::UpdateDropProb(bool overflow)
{
    NS_LOG_FUNCTION(this << overflow);

    Time now = Simulator::Now();
    if (now - m_lastUpdate < m_freezeTime)
    {
        return;  // Too soon to update
    }

    if (overflow)
    {
        m_dropProb += m_increment;
        if (m_dropProb > 1.0)
        {
            m_dropProb = 1.0;
        }
    }
    else if (GetInternalQueue(0)->IsEmpty())
    {
        m_dropProb -= m_decrement;
        if (m_dropProb < 0.0)
        {
            m_dropProb = 0.0;
        }
    }

    m_lastUpdate = now;
    NS_LOG_DEBUG("Updated drop probability: " << m_dropProb);
}

Ptr<QueueDiscItem>
BlueQueueDisc::DoDequeue()
{
    NS_LOG_FUNCTION(this);

    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        UpdateDropProb(false);  // Underflow event
        return nullptr;
    }

    Ptr<QueueDiscItem> item = GetInternalQueue(0)->Dequeue();
    NS_LOG_LOGIC("Popped " << item);
    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return item;
}

Ptr<const QueueDiscItem>
BlueQueueDisc::DoPeek()
{
    NS_LOG_FUNCTION(this);
    if (GetInternalQueue(0)->IsEmpty())
    {
        NS_LOG_LOGIC("Queue empty");
        return nullptr;
    }

    Ptr<const QueueDiscItem> item = GetInternalQueue(0)->Peek();
    NS_LOG_LOGIC("Number packets " << GetInternalQueue(0)->GetNPackets());
    NS_LOG_LOGIC("Number bytes " << GetInternalQueue(0)->GetNBytes());

    return item;
}

bool
BlueQueueDisc::CheckConfig()
{
    NS_LOG_FUNCTION(this);
    if (GetNQueueDiscClasses() > 0)
    {
        NS_LOG_ERROR("BlueQueueDisc cannot have classes");
        return false;
    }

    if (GetNPacketFilters() > 0)
    {
        NS_LOG_ERROR("BlueQueueDisc cannot have packet filters");
        return false;
    }

    if (GetNInternalQueues() == 0)
    {
        // Add a DropTail queue
        AddInternalQueue(
            CreateObjectWithAttributes<DropTailQueue<QueueDiscItem>>(
                "MaxSize", QueueSizeValue(GetMaxSize())));
    }

    if (GetNInternalQueues() != 1)
    {
        NS_LOG_ERROR("BlueQueueDisc needs 1 internal queue");
        return false;
    }

    return true;
}
} // namespace ns3