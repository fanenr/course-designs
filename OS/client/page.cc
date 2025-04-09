#include "ui_page.h"
#include <queue>

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

  std::queue<entry *> queue;
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

    queue.push (&entry);
    auto n = queue.size ();
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

    queue.pop ();
    entry->present = false;
    entry->modified = false;
    entry->frame_number = (size_t)-1;

    return frame;
  }
};

int
main (int argc, char **argv)
{
  auto app = QApplication (argc, argv);
  auto win = QMainWindow ();
  auto ui = Ui::window ();
  ui.setupUi (&win);

  win.show ();
  return app.exec ();
}
