#include "MessagePersister.h"

#include <sstream>

namespace
{
const std::string DELIMITER = "\n";
}

namespace wolkabout
{
std::string MessagePersister::save(const wolkabout::Message& message) const
{
    std::stringstream stream;

    stream << message.getChannel() << DELIMITER << message.getContent();

    return stream.str();
}

std::unique_ptr<Message> MessagePersister::load(const std::string& text) const
{
    const auto pos = text.find(DELIMITER);

    if (pos == std::string::npos)
    {
        return nullptr;
    }

    const auto channel = text.substr(0, pos);
    const auto content = text.substr(pos + DELIMITER.size());

    return std::unique_ptr<Message>(new Message(channel, content));
}
}    // namespace wolkabout
