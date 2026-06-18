#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Render-settings fingerprint. These are exactly the fields Section stores in
// its cache header (see Section.cpp). Page counts are only valid for one
// fingerprint; when any field changes the section caches and this map are
// invalidated together.
struct PageMapFingerprint {
  int fontId = 0;
  float lineCompression = 0.0f;
  bool extraParagraphSpacing = false;
  uint8_t paragraphAlignment = 0;
  uint16_t viewportWidth = 0;
  uint16_t viewportHeight = 0;
  bool hyphenationEnabled = false;
  bool embeddedStyle = false;
  uint8_t imageRendering = 0;
  bool focusReadingEnabled = false;

  bool operator==(const PageMapFingerprint& o) const;
};

// Tracks per-section (spine item) page counts for a whole book so the reader can
// show book-global "page X of Y". Sections that have been paginated hold an exact
// count; the rest are estimated from their byte size, calibrated against the
// paginated ones. Renderer-/settings-independent: the reader passes byte sizes
// and the fingerprint in.
class BookPageMap {
 public:
  // sectionBytes[i] = uncompressed byte size of spine item i. Resets all counts
  // to unknown.
  void init(std::vector<uint32_t> sectionBytes, const PageMapFingerprint& fingerprint);

  // Mark a section's exact page count (0 is a valid count: an empty chapter).
  // Returns true if the stored value changed.
  bool recordSection(int index, uint16_t pageCount);

  // Book-global current page (1-based) = sum of counts of sections before
  // spineIndex (exact or estimated) + pageInSection + 1.
  int globalPage(int spineIndex, int pageInSection) const;

  // Book-global total pages (exact + estimated).
  int total() const;

  // True once every section has an exact count.
  bool isExact() const;

  // Lowest-index section still unknown at/after fromIndex, or -1 if none.
  int nextUnknown(int fromIndex = 0) const;

  // Persistence via HalStorage. load() returns false and leaves all counts
  // unknown if the file is missing, the version/fingerprint/spine-count differ.
  bool load(const std::string& path);
  bool save(const std::string& path) const;

  const PageMapFingerprint& fingerprint() const { return fingerprint_; }
  int sectionCount() const { return static_cast<int>(pages_.size()); }

 private:
  static constexpr int32_t UNKNOWN = -1;

  std::vector<int32_t> pages_;  // UNKNOWN(-1), or exact page count (>= 0)
  std::vector<uint32_t> sectionBytes_;
  PageMapFingerprint fingerprint_;

  // Average bytes-per-page from paginated sections (count > 0); a coarse seed
  // before any section is known.
  float bytesPerPage() const;
  // Exact count if known, else byte-estimate (>= 1).
  int pagesOrEstimate(int index, float bytesPerPage) const;
};
