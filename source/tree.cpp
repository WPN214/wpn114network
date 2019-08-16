#include "tree.hpp"

//-------------------------------------------------------------------------------------------------
void
Node::componentComplete()
//-------------------------------------------------------------------------------------------------
{
    if (m_name.isNull())
        m_name = m_path.split('/').last();

    if (!m_tree)
        m_tree = Tree::instance();

    assert(m_tree);
    m_tree->link(this);
}
