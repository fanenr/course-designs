#include <algorithm>
#include <cstdio>
#include <list>

struct memory_allocator
{
  struct block
  {
    bool free;
    void *data;
    size_t size;
  };

  enum class policy
  {
    best,
    first,
    worst,
  };

  std::list<block> blocks = {};

  memory_allocator () { blocks.push_back ({ true, new char[4096], 4096 }); }

  void *
  alloc (size_t size, policy policy)
  {
    auto iter = blocks.end ();

    switch (policy)
      {
      case policy::best:
        iter = best_block (size);
        break;
      case policy::first:
        iter = first_block (size);
        break;
      case policy::worst:
        iter = worst_block (size);
        break;
      }

    if (iter != blocks.end ())
      {
        iter->free = false;
        if (iter->size == size)
          return iter->data;

        auto blk = block{ true, (char *)iter->data + size, iter->size - size };
        iter->size = size;

        blocks.insert (std::next (iter), blk);
        return iter->data;
      }

    if (size < 4096)
      {
        auto data = new char[4096];
        auto blk1 = block{ false, data, size };
        auto blk2 = block{ true, data + size, 4096 - size };
        blocks.push_back (blk1);
        blocks.push_back (blk2);
        return data;
      }

    auto blk = block{ false, new char[size], size };
    blocks.push_back (blk);
    return blk.data;
  }

  void
  free (void *ptr)
  {
    auto it = std::find_if (blocks.begin (), blocks.end (),
                            [=] (auto const &blk) { return blk.data == ptr; });
    if (it == blocks.end ())
      throw "bad ptr";

    auto &blk = *it;
    blk.free = true;

    auto next = std::next (it);
    if (next != blocks.end () && next->free)
      {
        blk.size += next->size;
        blk.data = new char[blk.size];
        blocks.erase (next);
      }

    auto prev = std::prev (it);
    if (it != blocks.begin () && prev->free)
      {
        prev->size += blk.size;
        prev->data = new char[prev->size];
        blocks.erase (it);
      }
  }

private:
  decltype (blocks)::iterator first_block (size_t size)
  {
    for (auto it = blocks.begin (); it != blocks.end (); it++)
      if (it->free && it->size >= size)
        return it;
    return blocks.end ();
  }

  decltype (blocks)::iterator best_block (size_t size)
  {
    auto iter = blocks.end ();
    for (auto it = blocks.begin (); it != blocks.end (); it++)
      {
        if (!it->free || it->size < size)
          continue;
        if (iter == blocks.end () || it->size < iter->size)
          iter = it;
      }
    return iter;
  }

  decltype (blocks)::iterator worst_block (size_t size)
  {
    auto iter = blocks.end ();
    for (auto it = blocks.begin (); it != blocks.end (); it++)
      {
        if (!it->free || it->size < size)
          continue;
        if (iter == blocks.end () || it->size > iter->size)
          iter = it;
      }
    return iter;
  }
};

int
main (int argc, char **argv)
{
}
