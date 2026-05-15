
/******************************************************************************
 * MODULE     : picture_cache_test.cpp
 * DESCRIPTION: picture cache cleanup 的单元测试
 * COPYRIGHT  : (C) 2025
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include <QtTest/QtTest>

#include "base.hpp"
#include "picture.hpp"
#include "renderer.hpp"

class TestPictureCache : public QObject {
  Q_OBJECT

private slots:
  void init ();
  void test_picture_cache_clean_does_not_crash ();
  void test_picture_cache_reset_does_not_crash ();
  void test_cached_load_and_clean ();
};

void
TestPictureCache::init () {
  init_lolly ();
}

void
TestPictureCache::test_picture_cache_clean_does_not_crash () {
  // 验证空缓存状态下调用 picture_cache_clean 不会崩溃
  picture_cache_clean ();
  QVERIFY (true);
}

void
TestPictureCache::test_picture_cache_reset_does_not_crash () {
  // 验证空缓存状态下调用 picture_cache_reset 不会崩溃
  picture_cache_reset ();
  QVERIFY (true);
}

void
TestPictureCache::test_cached_load_and_clean () {
  // 验证 picture_cache_reserve / picture_cache_release / picture_cache_clean
  // 的完整流程不会崩溃
  url test_url= url ("test://dummy/image.png");

  picture_cache_reserve (test_url, 100, 100, "", PIXEL);
  picture_cache_release (test_url, 100, 100, "", PIXEL);

  // 此时该条目应在黑名单中，picture_cache_clean 可安全清理
  picture_cache_clean ();
  QVERIFY (true);
}

QTEST_MAIN (TestPictureCache)
#include "picture_cache_test.moc"
