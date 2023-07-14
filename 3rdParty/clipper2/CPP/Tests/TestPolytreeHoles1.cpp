#include <gtest/gtest.h>
#include "clipper2/clipper.h"
#include "ClipFileLoad.h"

using namespace Clipper2Lib;

TEST(Clipper2Tests, TestPolytreeHoles1)
{
  std::ifstream ifs("PolytreeHoleOwner.txt");
  ASSERT_TRUE(ifs);
  ASSERT_TRUE(ifs.good());

  Paths64 subject, subject_open, clip;
  PolyTree64 solution;
  Paths64 solution_open;
  ClipType ct;
  FillRule fr;
  int64_t area, count;

  bool success = false;
  ASSERT_TRUE(LoadTestNum(ifs, 1, subject, subject_open, clip, area, count, ct, fr));

  Clipper64 c;
  c.AddSubject(subject);
  c.AddOpenSubject(subject_open);
  c.AddClip(clip);
  c.Execute(ct, fr, solution, solution_open);

  EXPECT_TRUE(CheckPolytreeFullyContainsChildren(solution));

}
