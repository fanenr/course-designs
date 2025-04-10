#include "ui_page.h"
#include <deque>

struct page_table
{
  struct entry
  {
    bool present;
    bool modified;
    size_t page_number;
    size_t frame_number;
    size_t disk_location;
  };

  std::deque<entry *> queue;
  std::vector<entry> entries;

  const size_t vm_size;
  const size_t max_jobs;
  const size_t frame_size;

  page_table (size_t vm_size = 64 * 1024, size_t max_jobs = 4,
              size_t frame_size = 1024)
      : vm_size (vm_size), max_jobs (max_jobs), frame_size (frame_size)
  {
    auto n = vm_size / frame_size;
    for (size_t i = 0; i < n; i++)
      entries.push_back ({ false, false, i, (size_t)-1, i });
  }

  void
  access (size_t addr, bool mod)
  {
    auto pno = addr / frame_size;
    auto off = addr % frame_size;
    access (pno, off, mod);
  }

  void
  access (size_t pno, size_t off, bool mod)
  {
    auto &entry = entries.at (pno);
    if (entry.present)
      {
        if (mod)
          entry.modified = true;
        return;
      }

    auto n = queue.size ();
    queue.push_back (&entry);
    if (n < max_jobs)
      entry.frame_number = n;
    else
      entry.frame_number = free_page ();

    entry.present = true;
    if (mod)
      entry.modified = true;
  }

private:
  size_t
  free_page ()
  {
    auto entry = queue.front ();
    auto frame = entry->frame_number;

    queue.pop_front ();
    entry->present = false;
    entry->modified = false;
    entry->frame_number = (size_t)-1;

    return frame;
  }
};

static const QMap<QString, bool> ops = {
  { "+", true }, { "-", true },    { "*", true },
  { "/", true }, { "save", true }, { "load", false },
};

void
flush_table (QTableWidget *table, page_table const &pt)
{
  table->setRowCount (0);

  auto add_row = [=] (int row, page_table::entry const &pte) {
    auto frame = pte.frame_number;

    auto pno = new QTableWidgetItem (QString::number (pte.page_number));
    auto flg = new QTableWidgetItem (pte.present ? "命中" : "失效");
    auto fno = new QTableWidgetItem (
        frame == (size_t)-1 ? "" : QString::number (frame));
    auto mod = new QTableWidgetItem (pte.modified ? "已修改" : "未修改");
    auto loc = new QTableWidgetItem (QString::number (pte.disk_location));

    pno->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
    flg->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
    fno->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
    mod->setTextAlignment (Qt::AlignmentFlag::AlignCenter);
    loc->setTextAlignment (Qt::AlignmentFlag::AlignCenter);

    if (pte.present)
      flg->setForeground (QBrush (Qt::red));
    if (pte.modified)
      mod->setForeground (QBrush (Qt::red));

    table->insertRow (row);
    table->setItem (row, 0, pno);
    table->setItem (row, 1, flg);
    table->setItem (row, 2, fno);
    table->setItem (row, 3, mod);
    table->setItem (row, 4, loc);
  };

  auto row = 0;
  for (auto const &ppte : pt.queue)
    add_row (row++, *ppte);

  for (auto const &pte : pt.entries)
    {
      auto const &q = pt.queue;
      auto it = std::find (q.begin (), q.end (), &pte);
      if (it == q.end ())
        add_row (row++, pte);
    }
}

int
main (int argc, char **argv)
{
  auto app = QApplication (argc, argv);
  auto win = QMainWindow ();
  auto ui = Ui::window ();
  ui.setupUi (&win);

  for (auto const &op : ops.keys ())
    ui.combo->addItem (op);

  ui.table->setEditTriggers (QAbstractItemView::NoEditTriggers);
  ui.table->setSelectionBehavior (QAbstractItemView::SelectRows);
  ui.table->setSelectionMode (QAbstractItemView::SingleSelection);
  ui.table->horizontalHeader ()->setSectionResizeMode (QHeaderView::Stretch);
  ui.table->verticalHeader ()->setDefaultAlignment (
      Qt::AlignmentFlag::AlignCenter);

  ui.table->setColumnCount (5);
  ui.table->setHorizontalHeaderLabels (
      { "页号", "标志", "内存块号", "修改标志", "磁盘位置" });

  auto pt = page_table ();
  flush_table (ui.table, pt);

  QObject::connect (ui.btn, &QPushButton::clicked, [&] () {
    auto pno_str = ui.edit1->text ();
    auto off_str = ui.edit2->text ();
    auto addr_str = ui.edit3->text ();
    auto mod = ops[ui.combo->currentText ()];

    if (!pno_str.isEmpty () && !off_str.isEmpty ())
      {
        auto pno = pno_str.toUInt ();
        auto off = off_str.toUInt ();
        pt.access (pno, off, mod);
        flush_table (ui.table, pt);
        return;
      }

    if (!addr_str.isEmpty ())
      {
        auto addr = addr_str.toUInt ();
        pt.access (addr, mod);
        flush_table (ui.table, pt);
        return;
      }
  });

  win.show ();
  return app.exec ();
}
