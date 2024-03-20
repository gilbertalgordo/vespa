// Copyright Vespa.ai. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "datagram.h"
#include <ostream>

using document::BucketSpace;

namespace storage::api {

IMPLEMENT_COMMAND(MapVisitorCommand, MapVisitorReply)
IMPLEMENT_REPLY(MapVisitorReply)
IMPLEMENT_COMMAND(EmptyBucketsCommand, EmptyBucketsReply)
IMPLEMENT_REPLY(EmptyBucketsReply)

MapVisitorCommand::MapVisitorCommand()
    : StorageCommand(MessageType::MAPVISITOR)
{
}

void
MapVisitorCommand::print(std::ostream& out, bool verbose,
                         const std::string& indent) const
{
    out << "MapVisitor(" << _statistics.size() << " entries";
    if (verbose) {
        for (const auto & stat : _statistics) {
            out << ",\n" << indent << "  " << stat.first << ": "
                << vespalib::stringref(stat.second.c_str(), stat.second.length());
        }
        out << ") : ";
        StorageCommand::print(out, verbose, indent);
    } else {
        out << ")";
    }
}

MapVisitorReply::MapVisitorReply(const MapVisitorCommand& cmd)
    : StorageReply(cmd)
{
}

void
MapVisitorReply::print(std::ostream& out, bool verbose,
                       const std::string& indent) const
{
    out << "MapVisitorReply()";
    if (verbose) {
        out << " : ";
        StorageReply::print(out, verbose, indent);
    }
}

EmptyBucketsCommand::EmptyBucketsCommand(
        const std::vector<document::BucketId>& buckets)
    : StorageCommand(MessageType::EMPTYBUCKETS),
      _buckets(buckets)
{
}

void
EmptyBucketsCommand::print(std::ostream& out, bool verbose,
                           const std::string& indent) const
{
    out << "EmptyBuckets(";
    if (verbose) {
        for (const auto & bucket : _buckets) {
            out << "\n" << indent << "  ";
            out << bucket;
        }
    } else {
        out << _buckets.size() << " buckets";
    }
    out << ")";
    if (verbose) {
        out << " : ";
        StorageCommand::print(out, verbose, indent);
    }
}

EmptyBucketsReply::EmptyBucketsReply(const EmptyBucketsCommand& cmd)
    : StorageReply(cmd)
{
}

void
EmptyBucketsReply::print(std::ostream& out, bool verbose,
                         const std::string& indent) const
{
    out << "EmptyBucketsReply()";
    if (verbose) {
        out << " : ";
        StorageReply::print(out, verbose, indent);
    }
}

}
