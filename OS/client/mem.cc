#include "ui_mem.h"

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
    if (!size)
      return nullptr;

    if (size >= 4096)
      {
        auto blk = block{ false, new char[size], size };
        blocks.push_back (blk);
        return blk.data;
      }

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

    if (iter == blocks.end ())
      {
        auto data = new char[4096];
        blocks.push_back (block{ false, data, size });
        blocks.push_back (block{ true, data + size, 4096 - size });
        return data;
      }

    iter->free = false;
    if (iter->size != size)
      {
        auto blk = block{ true, (char *)iter->data + size, iter->size - size };
        iter->size = size;
        blocks.insert (std::next (iter), blk);
      }
    return iter->data;
  }

  void
  free (void *ptr)
  {
    auto iter
        = std::find_if (blocks.begin (), blocks.end (),
                        [=] (auto const &blk) { return ptr == blk.data; });
    if (iter == blocks.end ())
      throw "bad ptr";
    if (iter->free)
      return;

    iter->free = true;
    auto next = std::next (iter);
    if (next != blocks.end () && next->free)
      {
        iter->size += next->size;
        iter->data = new char[iter->size];
        blocks.erase (next);
      }

    auto prev = std::prev (iter);
    if (iter != blocks.begin () && prev->free)
      {
        prev->size += iter->size;
        prev->data = new char[prev->size];
        blocks.erase (iter);
      }
  }

private:
  decltype (blocks)::iterator
  first_block (size_t size)
  {
    for (auto it = blocks.begin (); it != blocks.end (); it++)
      if (it->free && it->size >= size)
        return it;
    return blocks.end ();
  }

  decltype (blocks)::iterator
  best_block (size_t size)
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

  decltype (blocks)::iterator
  worst_block (size_t size)
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

void
flush_table (QTableWidget *table, memory_allocator const &mem)
{
  table->setRowCount (0);

  auto row = 0;
  for (auto const &blk : mem.blocks)
    {
      auto sts = new QTableWidgetItem (blk.free ? "空闲" : "已分配");
      auto size = new QTableWidgetItem (QString::number (blk.size));
      auto data = new QTableWidgetItem (
          "0x" + QString::number ((size_t)blk.data), 16);

      sts->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
      size->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
      data->setTextAlignment (Qt::AlignmentFlag::AlignCenter);

      if (!blk.free)
        sts->setForeground (QBrush (Qt::red));

      table->insertRow (row);
      table->setItem (row, 2, sts);
      table->setItem (row, 1, size);
      table->setItem (row, 0, data);

      row++;
    }
}

int
main (int argc, char **argv)
{
  auto app = QApplication (argc, argv);
  auto win = QMainWindow ();
  auto ui = Ui::window ();
  ui.setupUi (&win);

  ui.combo->addItems ({ "最佳适应", "最先适应", "最坏适应" });
  auto valid = new QIntValidator (0, INT_MAX, &win);
  ui.edit->setValidator (valid);

  ui.table->setEditTriggers (QAbstractItemView::NoEditTriggers);
  ui.table->setSelectionBehavior (QAbstractItemView::SelectRows);
  ui.table->setSelectionMode (QAbstractItemView::SingleSelection);
  ui.table->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
  ui.table->verticalHeader ()->setDefaultAlignment (
      Qt::AlignmentFlag::AlignCenter);

  ui.table->setColumnCount (3);
  ui.table->setHorizontalHeaderLabels (
      { "内存块首地址", "内存块大小", "状态" });

  auto mem = memory_allocator ();
  flush_table (ui.table, mem);

  QObject::connect (ui.btn1, &QPushButton::clicked, [&] {
    auto input = ui.edit->text ().toUInt ();
    auto policy = (memory_allocator::policy)ui.combo->currentIndex ();
    mem.alloc (input, policy);
    flush_table (ui.table, mem);
  });

  QObject::connect (ui.btn2, &QPushButton::clicked, [&] {
    auto row = ui.table->currentRow ();
    if (row < 0)
      return;
    auto it = mem.blocks.begin ();
    for (; row; row--)
      it++;
    mem.free (it->data);
    flush_table (ui.table, mem);
  });

  QObject::connect (ui.btn3, &QPushButton::clicked, [&] {
    mem = memory_allocator ();
    flush_table (ui.table, mem);
  });

  win.show ();
  return app.exec ();
}
