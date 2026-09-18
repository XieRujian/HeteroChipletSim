#include <cassert>
#include <string>

#include "util/ini.h"
#include "util/random.h"

#include "network.h"
#include "arch.h"

namespace core
{
    void Network::LoadTopology(std::string json_path)
    {
        cJSON *json = cJSON_Parse(util::GetFileContent(json_path).c_str());
        cJSON *nodes = nullptr;
        cJSON *edges = nullptr;
        cJSON *chain = nullptr;
        for (cJSON *item = json->child; item; item = item->next)
        {
            std::string key(item->string);
            if (key == "nodes")
                nodes = item;
            else if (key == "edges")
                edges = item;
            else if (key == "chain")
                chain = item;
        }
        assert(nodes != nullptr);
        assert(edges != nullptr);
        assert(chain != nullptr);
        for (cJSON *item = nodes->child; item; item = item->next)
        {
            NodeId id = node_name_to_id_.size();
            std::string name(item->valuestring);
            node_name_to_id_[name] = id;
            node_names_.push_back(name);
        }
        in_chain_ids_by_node_ = std::vector<std::vector<ChainId>>(GetNumNodes());
        out_chain_ids_by_node_ = std::vector<std::vector<ChainId>>(GetNumNodes());
        for (cJSON *item = edges->child; item; item = item->next)
        {
            Edge edge;
            for (cJSON *prop = item->child; prop; prop = prop->next)
            {
                std::string key(prop->string);
                if (key == "from")
                    edge.from_node = node_name_to_id_.at(prop->valuestring);
                else if (key == "to")
                    edge.to_node = node_name_to_id_.at(prop->valuestring);
                else if (key == "bandwidth")
                    edge.bandwidth_bytes_per_tick = prop->valuedouble;
                else if (key == "energy_pj_per_bit")
                    edge.energy_pj_per_bit = prop->valuedouble;
            }
            edge_name_to_id_[std::string(item->string)] = edges_.size();
            edges_.push_back(edge);
        }
        chain_ids_contain_edge_ = std::vector<std::vector<ChainId>>(GetNumEdges());
        for (cJSON *item = chain->child; item; item = item->next)
        {
            Chain chain;
            ChainId c = chains_.size();
            for (cJSON *prop = item->child; prop; prop = prop->next)
            {
                std::string key(prop->string);
                if (key == "from")
                    chain.from_node = node_name_to_id_.at(prop->valuestring);
                else if (key == "to")
                    for (cJSON *to_node = prop->child; to_node; to_node = to_node->next)
                    {
                        NodeId to_node_id = node_name_to_id_.at(to_node->valuestring);
                        chain.to_nodes.push_back(to_node_id);
                        in_chain_ids_by_node_[to_node_id].push_back(c);
                    }
                else if (key == "edges")
                    for (cJSON *edge_id = prop->child; edge_id; edge_id = edge_id->next)
                    {
                        EdgeId e = edge_name_to_id_.at(edge_id->valuestring);
                        chain.edge_ids.push_back(e);
                        chain_ids_contain_edge_[e].push_back(c);
                    }
            }
            out_chain_ids_by_node_[chain.from_node].push_back(c);
            chains_.push_back(chain);
        }
    }

    void Network::LoadArch(std::string ini_path)
    {
        node_archs_ = std::vector<Arch>(GetNumNodes());
        auto configs = util::ReadIniConfig(ini_path);
        for (auto &[arch_name, arch_config] : configs)
        {
            NodeName node_name = arch_config["node"];
            NodeId id = node_name_to_id_.at(node_name);
            node_of_arch_[arch_name] = id;
            node_archs_[id] = Arch(arch_name, arch_config);
        }
    }

    void Network::ShuffleArchs()
    {
        util::shuffle(node_archs_);
        for (NodeId id = 0; id < GetNumNodes(); ++id)
            node_of_arch_[node_archs_[id].GetName()] = id;
    }

    double Network::GetChainEnergyPerBit(ChainId c) const
    {
        double energy_pj_per_bit = 0;
        for (EdgeId e : chains_[c].edge_ids)
            energy_pj_per_bit += edges_[e].energy_pj_per_bit;
        return energy_pj_per_bit;
    }

    const Arch &Network::GetArch(NodeId node) const { return node_archs_.at(node); }
    Arch &Network::GetArch(NodeId node) { return node_archs_.at(node); }

    const std::vector<Edge> Network::GetEdgesSrcNode(NodeId node) const
    {
        std::vector<Edge> edges;
        for (const Edge &edge : edges_)
            if (edge.from_node == node)
                edges.push_back(edge);
        return edges;
    }

    const std::vector<NodeId> Network::GetNodes(ArchType type) const
    {
        std::vector<NodeId> nodes;
        for (const auto &arch : node_archs_)
            if (arch->GetType() == type)
                nodes.push_back(node_of_arch_.at(arch.GetName()));
        return nodes;
    }
}