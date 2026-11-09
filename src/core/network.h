#ifndef NETWORK_H
#define NETWORK_H

#include <map>
#include <memory>
#include <vector>

#include "util/cJSON.h"
#include "util/common.h"

namespace core
{
    class Arch;
    enum class ArchType;

    struct Edge
    {
        NodeId from_node;
        NodeId to_node;
        double bandwidth_bytes_per_tick;
        double energy_pj_per_bit;
    };

    struct Chain
    {
        NodeId from_node;
        std::vector<NodeId> to_nodes;
        std::vector<EdgeId> edge_ids;
    };

    class Network
    {
        std::vector<std::string> node_names_;
        std::vector<Edge> edges_;
        std::vector<Chain> chains_;
        std::map<std::string, NodeId> node_name_to_id_;
        std::map<std::string, EdgeId> edge_name_to_id_;
        std::vector<std::vector<ChainId>> in_chain_ids_by_node_;
        std::vector<std::vector<ChainId>> out_chain_ids_by_node_;
        std::vector<std::vector<ChainId>> chain_ids_contain_edge_;

        std::vector<ArchName> node_archs_;

    public:
        Network() = default;

        void LoadTopology(std::string json_path);
        void SetArch(NodeId node, const ArchName &arch) { node_archs_.at(node) = arch; }

        const NodeName &GetNodeName(NodeId n) const { return node_names_.at(n); }
        NodeId GetNodeId(const NodeName &name) const { return node_name_to_id_.at(name); }
        NodeId GetNumNodes() const noexcept { return node_name_to_id_.size(); }
        EdgeId GetNumEdges() const noexcept { return edges_.size(); }
        ChainId GetNumChains() const noexcept { return chains_.size(); }
        const std::vector<Edge> &GetEdges() const noexcept { return edges_; }
        const std::vector<ChainId> &GetInChainIds(NodeId n) const noexcept { return in_chain_ids_by_node_[n]; }
        const std::vector<ChainId> &GetOutChainIds(NodeId n) const noexcept { return out_chain_ids_by_node_[n]; }
        const std::vector<ChainId> &GetChainIdsContainEdge(EdgeId e) const noexcept { return chain_ids_contain_edge_[e]; }
        const std::vector<EdgeId> &GetEdgeIdsInChain(ChainId c) const noexcept { return chains_[c].edge_ids; }
        const std::vector<Chain> &GetChains() const noexcept { return chains_; }
        const std::vector<Edge> GetEdgesSrcNode(NodeId node) const;
        const ArchName &GetArch(NodeId node) const { return node_archs_.at(node); }
        double GetChainEnergyPerBit(ChainId c) const;
    };
}

#endif // NETWORK_H