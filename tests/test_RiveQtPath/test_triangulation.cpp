#include <QObject>
#include <QTest>
#include <QDebug>

#include "riveqtpath.h"

class Test_PathTriangulation : public QObject
{
    Q_OBJECT

private slots:
    void test_calculateLineIntersection()
    {
        using Point = QVector2D;
        Point p1 = { 1, 1 };
        Point p2 = { 10, 1 };
        Point p3 = { 3, 1 };
        Point p4 = { 9, 2 };

        std::optional<Point> intersection = RiveQtPath::calculateLineIntersection(p1, p2, p3, p4);

        QVERIFY(!intersection.has_value());
    }

    void test_orientation()
    {
        QList<QVector2D> a { QVector2D(21.882, -31.38), QVector2D(21.44, -11.78), QVector2D(89.4851, -11.78) };
        QCOMPARE(RiveQtPath::orientation(a), 1);

        QList<QVector2D> a2 { QVector2D(21.882, -31.38), QVector2D(89.4851, -11.78), QVector2D(21.882, -11.78) };
        QCOMPARE(RiveQtPath::orientation(a2), 2);
    }

    void test_doTrianglesOverlap_data()
    {
        QTest::addColumn<QVector<QVector2D>>("triangle");
        QTest::addColumn<bool>("result");

        QVector2D t11(3, 3), t12(5, 3), t13(4, 4);
        QTest::newRow("case 1 : completely inside") << QVector<QVector2D> { t11, t12, t13 } << true;

        QVector2D t21(5, 0), t22(10, 9), t23(1, 9);
        QTest::newRow("case 2 : star configuration, no points covered") << QVector<QVector2D> { t21, t22, t23 } << true;

        QVector2D t31(3, 3), t32(13, 3), t33(8, 10); // case 3 : shifted
        QTest::newRow("case 3 : shifted") << QVector<QVector2D> { t31, t32, t33 } << true;

        QVector2D t41(5, 5), t42(6, 4), t43(10, 10); // case 4 : one edge inside
        QTest::newRow("case 4 : one edge inside") << QVector<QVector2D> { t41, t42, t43 } << true;

        QVector2D t51(3, 3), t52(7, 3), t53(5, 12); // case 5 : inside triangle covers one point
        QTest::newRow("case 5 : inside triangle covers one point") << QVector<QVector2D> { t51, t52, t53 } << true;

        QVector2D t61(0, 2), t62(0, 5), t63(10, 5); // case 6 : only the area covers, no points covered
        QTest::newRow("case 6 : only the area covers, no points covered") << QVector<QVector2D> { t61, t62, t63 } << true;

        QVector2D t71(3, 0), t72(7, 0), t73(13, 5); // case 7 : one corner covered, all edges cut
        QTest::newRow("case 7 : one corner covered, all edges cut") << QVector<QVector2D> { t71, t72, t73 } << true;

        QVector2D t81(15, 15), t82(20, 25), t83(25, 15); // case 8 : not covered
        QTest::newRow("case 8 : not covered") << QVector<QVector2D> { t81, t82, t83 } << false;

        QVector<QVector2D> c9 = { { 1, 1 }, { 10, 1 }, { 5, 10 } };
        QTest::newRow("case 9 : exactly covered") << c9 << true;

        QVector<QVector2D> c10 = { { 10, 1 }, { 5, 10 }, { 15, 15 } };
        QTest::newRow("case 10: share an edge, uncovered") << c10 << false;

        QVector<QVector2D> c11 = { { 5, 1 }, { 10, 1 }, { 5, 10 } };
        QTest::newRow("case 11: share 2 edges and points, overlapping") << c11 << true;

        QVector<QVector2D> c12 = { { 5, 1 }, { 6, 1 }, { 5, 2 } };
        QTest::newRow("case 12: share 1 edges, overlapping") << c12 << true;
    }

    void test_doTrianglesOverlap()
    {
        QFETCH(QVector<QVector2D>, triangle);
        QFETCH(bool, result);
        QVector2D p1(1, 1), p2(10, 1), p3(5, 10);

        QCOMPARE(RiveQtPath::doTrianglesOverlap(p1, p2, p3, triangle[0], triangle[1], triangle[2]), result);
    }

    void test_isInsidePolygon()
    {
        QVector<QVector2D> polygon = { QVector2D(0, 0), QVector2D(5, 0), QVector2D(5, 5), QVector2D(0, 5) };

        // Test point inside the polygon
        QVERIFY(RiveQtPath::isInsidePolygon(polygon, QVector2D(2, 3)));

        // Test point outside the polygon
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(6, 6)));

        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(2, 0)));

        // Test point on the edge of the polygon (should return false)
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(0, 0))); // Vertex of the polygon
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(2.5, 0))); // On the bottom edge
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(5, 2.5))); // On the right edge
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(2.5, 5))); // On the top edge
        QVERIFY(!RiveQtPath::isInsidePolygon(polygon, QVector2D(0, 2.5))); // On the left edge
    }

    void test_doTrianglesOverlap_edgeOverlap()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector2D t1(10, 5), t2(5, 1), t3(10, 1);
        QVERIFY2(RiveQtPath::doTrianglesOverlap(q1, q2, q3, t1, t2, t3), "case : share 2 edges one point, overlapping");

        QVector2D t11(10, 5), t12(5, 1), t13(10, 2);
        QVERIFY2(RiveQtPath::doTrianglesOverlap(q1, q2, q3, t11, t12, t13), "case : all points on edges, overlapping");
    }

    void test_findOverlappingTriangles()
    {
        QVector<QVector2D> trianglePoints = { { 1, 1 },   { 10, 1 },  { 5, 10 },  { 5, 5 },  { 15, 5 },
                                              { 10, 15 }, { 15, 15 }, { 20, 25 }, { 25, 15 } };
        QVector<std::pair<size_t, size_t>> expected { { 0, 1 } };
        const auto &result = RiveQtPath::findOverlappingTriangles(trianglePoints);

        QVERIFY(RiveQtPath::findOverlappingTriangles(trianglePoints).size() == expected.size());

        for (size_t i = 0; i < result.size(); ++i) {
            QCOMPARE(result.at(i), expected.at(i));
        }
    }

    void test_concaveHull_simpleCase()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(5, 10);
        QVector<QVector2D> t1 { t11, t12, t13 };

        QVector2D t21(3, 3), t22(5, 3), t23(4, 4);
        QVector<QVector2D> t2 { t21, t22, t23 };
        QVector<QVector2D> result;

        RiveQtPath::concaveHull(t2, t1, result);
        for (int i = 0; i < result.size(); ++i)
            QCOMPARE(result.at(i), t1.at(i));

        result.clear();
        RiveQtPath::concaveHull(t1, t2, result);
        for (int i = 0; i < result.size(); ++i)
            QCOMPARE(result.at(i), t1.at(i));
    }

    void test_concaveHull_starConfiguration()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(5, 10);
        QVector<QVector2D> t1 { t11, t12, t13 };

        QVector2D t21(5, 0), t22(10, 9), t23(1, 9);
        QVector<QVector2D> t2 { t22, t23, t21 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t1, t2, result);
        QCOMPARE(result.size(), 12);
    }

    void test_concaveHull_noPointsCovered()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(5, 10);
        QVector<QVector2D> t1 { t11, t12, t13 };

        QVector2D t61(0, 5), t62(0, 2), t63(10, 5); // case 6 : only the area covers, no points covered
        QVector<QVector2D> t2 { t61, t62, t63 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t1, t2, result);
        QCOMPARE(result.size(), 10);
    }

    void test_concaveHull_allEdgesCut()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(5, 10);
        QVector<QVector2D> t1 { t11, t12, t13 };

        QVector2D t71(3, 0), t72(7, 0), t73(5, 13); // case 7 : one corner covered, all edges cut
        QVector<QVector2D> t2 { t71, t72, t73 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t1, t2, result);
        QCOMPARE(result.size(), 9);
    }

    void test_concaveHull_simpleShift()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(5, 10);
        QVector<QVector2D> t1 { t11, t12, t13 };

        QVector2D t31(3, 3), t32(13, 3), t33(8, 10); // case 3 : shifted
        QVector<QVector2D> t2 { t31, t32, t33 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t1, t2, result);
        QCOMPARE(result.size(), 7);
    }

    void test_concaveHull_pointsOnEdges()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t1(3, 1), t2(9, 2), t3(9, 9);
        QVector<QVector2D> t { t1, t2, t3 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t0, t, result);
        QCOMPARE(result.size(), 3); // one point on point

        result.clear();
        QVector2D t11(10, 5), t12(5, 1), t13(10, 2);
        QVector<QVector2D> tt { t11, t12, t13 };

        RiveQtPath::concaveHull(t0, tt, result);
        QCOMPARE(result.size(), 3); // only points on edge
    }

    void test_concaveHull_polygonTest()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(10, 10), t14(1, 10);
        QVector<QVector2D> t1 { t11, t12, t13, t14 };

        QVector2D t21(3, 3), t22(5, 6), t23(12, 4);
        QVector<QVector2D> t2 { t21, t23, t22 };
        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t1, t2, result);
        //        qDebug() << result;
        QCOMPARE(result.size(), 7);
    }

    void test_concaveHull_polygonTestInversed()
    {
        QVector2D t11(1, 1), t12(10, 1), t13(10, 10), t14(1, 10);
        QVector<QVector2D> t1 { t11, t12, t13, t14 };

        QVector2D t21(3, 3), t22(5, 6), t23(12, 4);
        QVector<QVector2D> t2 { t21, t23, t22 };
        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t2, t1, result);
        //        qDebug() << result;
        QCOMPARE(result.size(), 7);
    }

    void test_concaveHull_polygonTestShiftedTwice()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t21(9, 7), t22(13, 8), t23(9, 8);
        QVector<QVector2D> t2 { t21, t22, t23 };
        QVector2D t31(4, 3), t32(13, 3), t33(8, 7);
        QVector<QVector2D> t3 { t31, t32, t33 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t0, t2, result);
        auto poly = result;
        result.clear();
        RiveQtPath::concaveHull(poly, t3, result);

        //        qDebug() << result;
        QCOMPARE(result.size(), 9);
    }

    void test_concaveHull_polygonTest_simpleCase()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t1(3, 1), t2(9, 2), t3(9, 9);
        QVector<QVector2D> t { t1, t2, t3 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t0, t, result);
        //        qDebug() << result;
        QCOMPARE(result.size(), 3);
    }

    void test_concaveHull_polygonTest_simpleShift()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t31(4, 3), t32(13, 3), t33(8, 7); // case 3 : shifted
        QVector<QVector2D> t2 { t31, t32, t33 };

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(t0, t2, result);
        qDebug() << result;
        QCOMPARE(result.size(), 6);
    }

    void test_removeOverlappingTriangles()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t1(3, 1), t2(9, 2), t3(9, 9);
        QVector<QVector2D> t { t1, t2, t3 };

        QVERIFY(RiveQtPath::doTrianglesOverlap(q1, q2, q3, t1, t2, t3));

        auto triangles = t0 + t;
        QVERIFY(triangles.size() >= 6);
        QVERIFY(!RiveQtPath::findOverlappingTriangles(triangles).empty());

        RiveQtPath::removeOverlappingTriangles(triangles);

        QVERIFY(!triangles.empty());
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
    }

    void test_removeOverlappingTriangles_simpleShift()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t31(4, 3), t32(13, 3), t33(8, 7); // case 3 : shifted
        QVector<QVector2D> t2 { t31, t32, t33 };

        auto triangles = t0 + t2;
        QVERIFY(!RiveQtPath::findOverlappingTriangles(triangles).empty());

        QVector<QVector2D> hull;
        RiveQtPath::concaveHull(t0, t2, hull);
        QCOMPARE(hull.size(), 6);

        RiveQtPath::removeOverlappingTriangles(triangles);
        QVERIFY(triangles.size() > 3);
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
    }

    void test_removeOverlappingTriangles_simpleShiftTwice()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t21(9, 7), t22(13, 8), t23(9, 8);
        QVector<QVector2D> t2 { t21, t22, t23 };
        QVector2D t31(4, 3), t32(13, 3), t33(8, 7);
        QVector<QVector2D> t3 { t31, t32, t33 };

        auto triangles = t0 + t2 + t3;
        QVERIFY(!RiveQtPath::findOverlappingTriangles(triangles).empty());

        RiveQtPath::removeOverlappingTriangles(triangles);
        QVERIFY(triangles.size() > 3);
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
        //        qDebug() << triangles;
    }

    void test_removeOverlappingTriangles_simpleShiftNestedOverlap()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t21(9, 4), t22(13, 5), t23(9, 8);
        QVector<QVector2D> t2 { t21, t22, t23 };
        QVector2D t31(4, 3), t32(13, 3), t33(8, 7);
        QVector<QVector2D> t3 { t31, t32, t33 };

        auto triangles = t0 + t2 + t3;
        QVERIFY(!RiveQtPath::findOverlappingTriangles(triangles).empty());

        RiveQtPath::removeOverlappingTriangles(triangles);
        QVERIFY(triangles.size() > 3);
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
        //        qDebug() << triangles;
    }

    void test_removeOverlappingTriangles_simpleShiftClockwiseTriangle()
    {
        QVector2D q1(1, 1), q2(10, 1), q3(10, 10);
        QVector<QVector2D> t0 { q1, q2, q3 };
        QVector2D t21(9, 4), t22(13, 5), t23(9, 8);
        QVector<QVector2D> t2 { t21, t23, t22 };

        auto triangles = t0 + t2;
        QVERIFY(!RiveQtPath::findOverlappingTriangles(triangles).empty());

        RiveQtPath::removeOverlappingTriangles(triangles);
        QVERIFY(triangles.size() > 3);
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
    }

    void test_concaveHull_crash()
    {
        QVector<QVector2D> a { QVector2D(21.882, -31.38), QVector2D(89.4851, -11.78), QVector2D(21.882, -11.78) };
        QVector<QVector2D> b { QVector2D(31.682, -21.58), QVector2D(12.082, -21.58), QVector2D(31.682, -22.8706) };

        // this does nothing here
        RiveQtPath::FixWinding(a[0], a[1], a[2]);
        RiveQtPath::FixWinding(b[0], b[1], b[2]);

        QVector<QVector2D> result;
        RiveQtPath::concaveHull(a, b, result);
        qDebug() << result;
        // FIXME: for some reason the intersections are not found.
        QCOMPARE(result.size(), 6);
    }

    void test_removeOverlappingTriangles_clockwiseTriangle()
    {
        QVector<QVector2D> t0 {
            QVector2D(31.682, -21.58),    QVector2D(12.082, -21.58),    QVector2D(31.682, -22.8706),  QVector2D(31.682, -22.8706),
            QVector2D(12.082, -22.8706),  QVector2D(12.082, -21.58),    QVector2D(31.682, -22.8706),  QVector2D(12.082, -22.8706),
            QVector2D(31.682, -26.4665),  QVector2D(31.682, -26.4665),  QVector2D(12.082, -26.4665),  QVector2D(12.082, -22.8706),
            QVector2D(31.682, -26.4665),  QVector2D(12.082, -26.4665),  QVector2D(31.682, -31.9536),  QVector2D(31.682, -31.9536),
            QVector2D(12.082, -31.9536),  QVector2D(12.082, -26.4665),  QVector2D(31.682, -31.9536),  QVector2D(12.082, -31.9536),
            QVector2D(31.682, -38.9179),  QVector2D(31.682, -38.9179),  QVector2D(12.082, -38.9179),  QVector2D(12.082, -31.9536),
            QVector2D(31.682, -38.9179),  QVector2D(12.082, -38.9179),  QVector2D(31.682, -46.9453),  QVector2D(31.682, -46.9453),
            QVector2D(12.082, -46.9453),  QVector2D(12.082, -38.9179),  QVector2D(31.682, -46.9453),  QVector2D(12.082, -46.9453),
            QVector2D(31.682, -55.6217),  QVector2D(31.682, -55.6217),  QVector2D(12.082, -55.6217),  QVector2D(12.082, -46.9453),
            QVector2D(31.682, -55.6217),  QVector2D(12.082, -55.6217),  QVector2D(31.682, -64.5331),  QVector2D(31.682, -64.5331),
            QVector2D(12.082, -64.5331),  QVector2D(12.082, -55.6217),  QVector2D(31.682, -64.5331),  QVector2D(12.082, -64.5331),
            QVector2D(31.682, -73.2655),  QVector2D(31.682, -73.2655),  QVector2D(12.082, -73.2655),  QVector2D(12.082, -64.5331),
            QVector2D(31.682, -73.2655),  QVector2D(12.082, -73.2655),  QVector2D(31.6819, -81.4048), QVector2D(31.6819, -81.4048),
            QVector2D(12.0819, -81.4048), QVector2D(12.082, -73.2655),  QVector2D(31.6819, -81.4048), QVector2D(12.0819, -81.4048),
            QVector2D(21.8819, -78.7369), QVector2D(21.8819, -78.7369), QVector2D(21.8819, -98.3369), QVector2D(12.0819, -81.4048),
            QVector2D(12.0819, -81.4048), QVector2D(12.0819, -99.4121), QVector2D(22.8984, -98.2841), QVector2D(12.0819, -81.4048),
            QVector2D(21.8819, -88.5369), QVector2D(22.8984, -98.2841), QVector2D(21.8819, -78.7369), QVector2D(21.8819, -98.3369),
            QVector2D(37.7199, -77.0986), QVector2D(37.7199, -77.0986), QVector2D(41.9992, -96.2257), QVector2D(21.8819, -98.3369),
            QVector2D(37.7199, -77.0986), QVector2D(41.9992, -96.2257), QVector2D(49.9446, -72.8096), QVector2D(49.9446, -72.8096),
            QVector2D(58.7162, -90.3372), QVector2D(41.9992, -96.2257), QVector2D(49.9446, -72.8096), QVector2D(58.7162, -90.3372),
            QVector2D(59.3215, -66.6127), QVector2D(59.3215, -66.6127), QVector2D(72.0283, -81.5358), QVector2D(58.7162, -90.3372),
            QVector2D(59.3215, -66.6127), QVector2D(72.0283, -81.5358), QVector2D(66.4803, -59.0259), QVector2D(66.4803, -59.0259),
            QVector2D(82.0662, -70.9103), QVector2D(72.0283, -81.5358), QVector2D(66.4803, -59.0259), QVector2D(82.0662, -70.9103),
            QVector2D(71.7943, -50.569),  QVector2D(71.7943, -50.569),  QVector2D(89.2168, -59.5479), QVector2D(82.0662, -70.9103),
            QVector2D(71.7943, -50.569),  QVector2D(89.2168, -59.5479), QVector2D(75.504, -41.9065),  QVector2D(75.504, -41.9065),
            QVector2D(94.0002, -48.3912), QVector2D(89.2168, -59.5479), QVector2D(75.504, -41.9065),  QVector2D(94.0002, -48.3912),
            QVector2D(77.8486, -33.8196), QVector2D(77.8486, -33.8196), QVector2D(96.9377, -38.2657), QVector2D(94.0002, -48.3912),
            QVector2D(77.8486, -33.8196), QVector2D(96.9377, -38.2657), QVector2D(79.1097, -27.1406), QVector2D(79.1097, -27.1406),
            QVector2D(98.5079, -29.9464), QVector2D(96.9377, -38.2657), QVector2D(79.1097, -27.1406), QVector2D(98.5079, -29.9464),
            QVector2D(79.6076, -22.7122), QVector2D(79.6076, -22.7122), QVector2D(99.1512, -24.1974), QVector2D(98.5079, -29.9464),
            QVector2D(79.6076, -22.7122), QVector2D(99.1512, -24.1974), QVector2D(79.7133, -20.8374), QVector2D(79.7133, -20.8374),
            QVector2D(99.2569, -22.3226), QVector2D(99.1512, -24.1974), QVector2D(99.1512, -24.1974), QVector2D(99.8512, -11.78),
            QVector2D(89.4851, -11.78),   QVector2D(99.1512, -24.1974), QVector2D(89.4851, -21.58),   QVector2D(89.4851, -11.78),
            QVector2D(89.4851, -31.38),   QVector2D(89.4851, -11.78),   QVector2D(21.882, -31.38),    QVector2D(21.882, -31.38),
            QVector2D(21.882, -11.78),    QVector2D(89.4851, -11.78),   QVector2D(89.4851, -11.78),   QVector2D(12.082, -11.78),
            QVector2D(12.082, -21.58),    QVector2D(89.4851, -11.78),   QVector2D(21.882, -21.58),    QVector2D(12.082, -21.58)
        };

        auto triangles = t0;

        RiveQtPath::removeOverlappingTriangles(triangles);
        QVERIFY(triangles.size() > 3);
        QVERIFY(RiveQtPath::findOverlappingTriangles(triangles).empty());
    }
};

QTEST_MAIN(Test_PathTriangulation)

#include "test_triangulation.moc"
