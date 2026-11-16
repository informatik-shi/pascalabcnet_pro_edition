// Native implementation of the ordering in SyntaxVisitors/SugarVisitors/AssignTuplesDesugarVisitor.
#include "tuple_assignments.hpp"
#include <algorithm>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>

namespace abc {
namespace {
using Kind=TupleOperandKind;
using Source=TupleOperandSource;
struct Symbol {
    TupleOperand operand;
    TupleOperandRef ref;
};
struct Aliases {
    std::vector<std::size_t> targets,values;
};
struct Vertex {
    TupleOperandRef ref;
    std::vector<std::size_t> outgoing;
    std::optional<std::size_t> incoming;
};

// Insertion order is observable when dependent reads or writes call properties.
std::vector<std::size_t> postorder(const std::vector<Vertex>& graph,bool roots_only) {
    struct Frame {std::size_t vertex,next=0;};
    std::vector<bool> visited(graph.size());
    std::vector<Frame> stack;
    std::vector<std::size_t> order;
    for(std::size_t root=0;root<graph.size();++root){
        if(visited[root] || (roots_only && graph[root].incoming))continue;
        visited[root]=true;
        stack.push_back({root});
        while(!stack.empty()){
            auto& frame=stack.back();
            const auto& children=graph[frame.vertex].outgoing;
            if(frame.next==children.size()){
                order.push_back(frame.vertex);
                stack.pop_back();
                continue;
            }
            const auto child=children[frame.next++];
            if(!visited[child]){
                visited[child]=true;
                stack.push_back({child});
            }
        }
    }
    return order;
}
}

std::vector<TupleAssignment> order_tuple_assignments(const std::vector<TupleOperand>& targets,
    const std::vector<TupleOperand>& values) {
    if(targets.size()>values.size())throw std::logic_error("Not enough tuple assignment values");
    std::vector<Symbol> left,right;
    for(std::size_t i=0;i<targets.size();++i)left.push_back({targets[i],{Source::target,i}});
    for(std::size_t i=0;i<values.size();++i)right.push_back({values[i],{Source::value,i}});
    std::size_t temporaries=0;
    const auto temporary=[&] {
        const auto index=temporaries++;
        return Symbol{{Kind::local,"$tuple_"+std::to_string(index),{}},{Source::temporary,index}};
    };
    std::vector<TupleAssignment> order;
    std::set<std::string> seen;
    for(std::size_t i=0;i<left.size();++i){
        if(seen.insert(left[i].operand.identity).second && left[i].operand.kind!=Kind::pointer
            && right[i].operand.kind!=Kind::pointer)continue;
        // Repeated destinations and pointers capture every right operand first.
        for(auto& value:right){
            const auto captured=temporary();
            order.push_back({captured.ref,value.ref});
            value=captured;
        }
        for(std::size_t n=0;n<left.size();++n)order.push_back({left[n].ref,right[n].ref});
        return order;
    }

    std::vector<TupleAssignment> write_temporaries;
    const auto keep_value=[&](std::size_t i){
        const auto saved=temporary();
        order.push_back({saved.ref,right[i].ref});
        right[i]=saved;
    };
    const auto defer_target=[&](std::size_t i){
        const auto saved=temporary();
        write_temporaries.push_back({left[i].ref,saved.ref});
        left[i]=saved;
    };
    for(std::size_t i=0;i<left.size();++i){
        if(right[i].operand.kind==Kind::expression || right[i].operand.kind==Kind::by_reference)keep_value(i);
        if(left[i].operand.kind==Kind::by_reference)defer_target(i);
    }
    for(std::size_t i=left.size();i>0;){
        --i;
        if(left[i].operand.identity!=right[i].operand.identity)continue;
        order.push_back({left[i].ref,right[i].ref});
        left.erase(left.begin()+static_cast<std::ptrdiff_t>(i));
        right.erase(right.begin()+static_cast<std::ptrdiff_t>(i));
    }

    std::vector<Aliases> groups;
    std::map<std::string,std::size_t> by_name;
    Aliases indexed;
    const auto group_for=[&](const std::string& name)->Aliases& {
        const auto [position,created]=by_name.emplace(name,groups.size());
        if(created)groups.emplace_back();
        return groups[position->second];
    };
    const auto add_alias=[&](const TupleOperand& operand,std::size_t i,bool target){
        Aliases* aliases=nullptr;
        if(operand.kind==Kind::index)aliases=&indexed;
        else if(operand.kind==Kind::member || operand.kind==Kind::outer)aliases=&group_for(operand.name);
        if(aliases)(target ? aliases->targets : aliases->values).push_back(i);
    };
    for(std::size_t i=0;i<left.size();++i){
        add_alias(left[i].operand,i,true);
        add_alias(right[i].operand,i,false);
    }
    const auto indexed_group=groups.size();
    groups.push_back(std::move(indexed));
    std::set<std::string> left_keys,right_keys;
    for(const auto& item:left)left_keys.insert(item.operand.identity);
    for(const auto& item:right)right_keys.insert(item.operand.identity);
    std::vector<TupleAssignment> before;
    std::deque<TupleAssignment> after;
    std::vector<bool> moved(left.size());
    const auto move=[&](std::size_t i,bool early){
        const TupleAssignment step{left[i].ref,right[i].ref};
        if(early)before.push_back(step);
        else after.push_front(step);
        moved[i]=true;
    };
    for(std::size_t i=0;i<left.size();++i)
        if(right[i].operand.kind==Kind::constant)move(i,false);

    const auto independent=[&](const TupleOperand& operand,bool check_targets){
        const auto& keys=check_targets ? left_keys : right_keys;
        if(operand.kind==Kind::local)return !keys.contains(operand.identity);
        if(operand.kind==Kind::outer && keys.contains(operand.identity))return false;
        if(operand.kind!=Kind::outer && operand.kind!=Kind::member && operand.kind!=Kind::index)return false;
        const auto group=operand.kind==Kind::index ? indexed_group : by_name.at(operand.name);
        const auto& aliases=groups[group];
        return check_targets ? aliases.targets.empty() : aliases.values.empty();
    };
    const auto defer_aliases=[&](const std::vector<std::size_t>& selected,const std::vector<std::size_t>& opposite,bool early){
        std::optional<std::size_t> directly;
        for(const auto i:selected){
            if(!directly && std::find(opposite.begin(),opposite.end(),i)!=opposite.end())directly=i;
            else if(early)keep_value(i);
            else defer_target(i);
        }
        if(directly)move(*directly,early);
    };
    for(auto& group:groups){
        for(const auto i:group.targets)if(independent(right[i].operand,true))move(i,false);
        for(const auto i:group.values)if(independent(left[i].operand,false))move(i,true);
        std::erase_if(group.targets,[&](std::size_t i){return moved[i];});
        std::erase_if(group.values,[&](std::size_t i){return moved[i];});
        if(group.targets.empty() || group.values.empty())continue;
        if(group.targets.size()<group.values.size()){
            defer_aliases(group.targets,group.values,false);
            group.targets.clear();
        }else{
            defer_aliases(group.values,group.targets,true);
            group.values.clear();
        }
    }
    order.insert(order.end(),before.begin(),before.end());
    after.insert(after.end(),write_temporaries.begin(),write_temporaries.end());

    // Build the dependency graph in source order, using the destination syntax
    // as the canonical reference when a symbol occurs on both sides.
    std::map<std::string,TupleOperandRef> canonical;
    for(std::size_t i=0;i<left.size();++i)if(!moved[i])canonical.emplace(left[i].operand.identity,left[i].ref);
    for(const auto& value:right)canonical.emplace(value.operand.identity,value.ref);
    std::vector<Vertex> graph;
    std::map<std::string,std::size_t> vertex_ids;
    const auto vertex=[&](const Symbol& symbol) {
        const auto& key=symbol.operand.identity;
        const auto [position,created]=vertex_ids.emplace(key,graph.size());
        if(created)graph.push_back({canonical.at(key),{},{}});
        return position->second;
    };
    for(std::size_t i=0;i<left.size();++i){
        if(moved[i])continue;
        const auto from=vertex(right[i]);
        const auto to=vertex(left[i]);
        graph[from].outgoing.push_back(to);
        graph[to].incoming=from;
    }
    auto finished=postorder(graph,false);
    std::vector<bool> visited(graph.size());
    for(auto it=finished.rbegin();it!=finished.rend();++it){
        const auto start=*it;
        if(visited[start])continue;
        auto node=start;
        do {
            visited[node]=true;
            if(!graph[node].incoming)break;
            node=*graph[node].incoming;
        } while(!visited[node]);
        if(node!=start || !graph[start].incoming)continue;
        const auto from=*graph[start].incoming;
        const auto saved=temporary();
        order.push_back({saved.ref,graph[from].ref});
        after.push_back({graph[start].ref,saved.ref});
        std::erase(graph[from].outgoing,start);
        graph[start].incoming.reset();
    }
    for(const auto node:postorder(graph,true))
        if(const auto from=graph[node].incoming)order.push_back({graph[node].ref,graph[*from].ref});
    order.insert(order.end(),after.begin(),after.end());
    return order;
}
}
