#include "canonization.hpp"
NormNode::NormNode(Node *_node,std::map<EventType::event_type_t, bool> &key_events)
{
    node = _node;
    norm_group = new NormGroup(node->get_group(), key_events);
}

NormNode::~NormNode()
{
    if (norm_group != nullptr)
        delete norm_group;
}

bool NormNode::is_empty()
{
    return norm_group->is_empty();
}

bool NormNode::operator<=(NormNode &other)
{
    if (*norm_group <= *(other.norm_group))
        return true;
    return false;
}

bool NormNode::operator==(NormNode &other)
{
	int overall_size = norm_group->normalized_size();
	if (overall_size == 0)
		return false;
#ifdef SLOW_DIST
	int dist = distace_to(other);
	if (dist < 0.2 * overall_size)
		return true;
	return false;
#else
    if (*norm_group == *(other.norm_group))
        return true;
    return false;
#endif
}

bool NormNode::operator!=(NormNode &other)
{
    return !((*this) == other);
}

int NormNode::edit(std::string str1, std::string str2, int m, int n)
{
	if (m == 0 || n == 0)
		return std::max(m, n);
	if (str1[m-1] == str2[n-1])
		return edit(str1, str2, m - 1, n - 1);
	return 1 + std::min(edit(str1, str2, m - 1, n - 1),
		std::min(edit(str1, str2, m, n - 1), edit(str1, str2, m - 1, n)));
}

int NormNode::distance_to(NormNode &other)
{
	std::string sig = signature();
	std::string peer = other.signature();
	return edit(sig, peer, sig.size(), peer.size());
}
