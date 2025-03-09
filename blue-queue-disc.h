#ifndef BLUE_QUEUE_DISC_H
#define BLUE_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/nstime.h"
#include "ns3/random-variable-stream.h"

namespace ns3 {

/**
 * @ingroup traffic-control
 *
 * @brief A BLUE packet queue disc
 */
class BlueQueueDisc : public QueueDisc {
public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief BlueQueueDisc Constructor
     *
     * Creates a BLUE queue disc
     */
    BlueQueueDisc();

    /**
     * @brief Destructor
     */
    ~BlueQueueDisc() override;

    // Reasons for dropping packets
    static constexpr const char* FORCED_DROP = "Forced drop";     //!< Queue full drop
    static constexpr const char* PROB_DROP = "Probabilistic drop"; //!< Random drop based on probability
    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

protected:
    /**
     * @brief Dispose of the object
     */
    void DoDispose() override;

private:
    bool DoEnqueue(Ptr<QueueDiscItem> item) override;
    Ptr<QueueDiscItem> DoDequeue() override;
    Ptr<const QueueDiscItem> DoPeek() override;
    bool CheckConfig() override;

    /**
     * @brief Initialize the queue parameters
     */
    void InitializeParams() override;

    /**
     * @brief Update the drop probability based on queue events
     * @param overflow True if queue is full, false if dequeue occurs on empty queue
     */
    void UpdateDropProb(bool overflow);

    // ** Variables supplied by user
    double m_increment;      //!< Drop probability increment on overflow
    double m_decrement;      //!< Drop probability decrement on underflow
    Time m_freezeTime;       //!< Time interval between drop probability updates

    // ** Variables maintained by BLUE
    double m_dropProb;       //!< Current drop probability
    Time m_lastUpdate;       //!< Last time drop probability was updated
    Ptr<UniformRandomVariable> m_uv; //!< Random number generator stream
};

} // namespace ns3

#endif // BLUE_QUEUE_DISC_H
