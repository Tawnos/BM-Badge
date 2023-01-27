#include "mage_geometry.h"
#include "mage_tileset.h"
#include "FrameBuffer.h"
#include "convert_endian.h"
#include "shim_err.h"
#include <algorithm>

MageGeometry::MageGeometry(uint32_t& offset)
{
#ifndef DC801_EMBEDDED
   ROM()->Read(name, offset, 32);
#else
   //skip over name:
   offset += 32;
#endif
   ROM()->Read(typeId, offset);
   ROM()->Read(pointCount, offset);
   ROM()->Read(segmentCount, offset);

   offset += 1; //padding

   //read pathLength:
   ROM()->Read(pathLength, offset);

   auto pointArrayPtr = new Point[pointCount]{ 0,0 };
   auto segmentLengthArrayPtr = new float[segmentCount] {0.0f};

   //generate appropriately sized point array:
   ROM()->Read(*pointArrayPtr, offset, pointCount);

   //generate appropriately sized array:
   ROM()->Read(*segmentLengthArrayPtr, offset, segmentCount);

   points.reset(pointArrayPtr);
   segmentLengths.reset(segmentLengthArrayPtr);
}

MageGeometry::MageGeometry(MageGeometryType type, uint8_t numPoints)
   : pointCount(numPoints)
{
   typeId = type;
   auto segmentCount = typeId == MageGeometryType::Polygon ? numPoints : numPoints - 1;
   points = std::unique_ptr<Point[]>(new Point[numPoints]{ 0,0 });
   segmentLengths = std::unique_ptr<float[]>(new float[segmentCount] {0.0f});
}

MageGeometry MageGeometry::FlipByFlags(uint8_t flags, uint16_t width, uint16_t height) const
{
   auto geometry = MageGeometry{ typeId, pointCount };
   if (flags != 0)
   {
      for (uint8_t i = 0; i < pointCount; i++)
      {
         const_cast<Point&>(geometry.points[i]) = geometry.points[i].flipByFlags(flags, width, height);
      }
   }
   return geometry;
}

bool MageGeometry::isPointInGeometry(Point point) const
{
   if (typeId == MageGeometryType::Point)
   {
      return point == GetPoint(0);
   }
   else if (typeId == MageGeometryType::Polyline || typeId == MageGeometryType::Polygon)
   {
      uint8_t i, j;
      bool c = false;
      for (i = 0, j = GetPointCount() - 1; i < GetPointCount(); j = i++)
      {
         //get the points for i and j:
         Point points_i = GetPoint(i);
         Point points_j = GetPoint(j);
         //do the fancy check:
         if ((points_i.y >= point.y) != (points_j.y >= point.y)
            && point.x <= (points_j.x - points_i.x) * (point.y - points_i.y) / (points_j.y - points_i.y) + points_i.x)
         {
            c = !c;
         }
      }
      return c;
   }
   else
   {
      // it's not a known geometry type, so always return false.
      return false;
   }
}

// Returns true if collision has occurred, and if it has,
// sets the new value of intersectPoint.
// Ref: https://stackoverflow.com/a/385355
// Ref: https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
std::optional<Point> MageGeometry::getIntersectPointBetweenLineSegments(
   const Point& lineAPointA, const Point& lineAPointB,
   const Point& lineBPointA, const Point& lineBPointB
)
{
   float x1 = lineAPointA.x;
   float x2 = lineAPointB.x;
   float x3 = lineBPointA.x;
   float x4 = lineBPointB.x;
   float y1 = lineAPointA.y;
   float y2 = lineAPointB.y;
   float y3 = lineBPointA.y;
   float y4 = lineBPointB.y;

   float x12 = x1 - x2;
   float x34 = x3 - x4;
   float y12 = y1 - y2;
   float y34 = y3 - y4;

   float c = x12 * y34 - y12 * x34;

   if (fabs(c) > 0.01)
   {
      // Intersection
      float a = x1 * y2 - y1 * x2;
      float b = x3 * y4 - y3 * x4;

      float x = (a * x34 - b * x12) / c;
      float y = (a * y34 - b * y12) / c;

      float lineAXMin = MIN(x1, x2);
      float lineAXMax = MAX(x1, x2);
      float lineAYMin = MIN(y1, y2);
      float lineAYMax = MAX(y1, y2);
      float lineBXMin = MIN(x3, x4);
      float lineBXMax = MAX(x3, x4);
      float lineBYMin = MIN(y3, y4);
      float lineBYMax = MAX(y3, y4);

      // Determine if the intersection is inside the bounds of lineA AND lineB
      if (x >= lineAXMin && x <= lineAXMax
       && y >= lineAYMin && y <= lineAYMax 
       && x >= lineBXMin && x <= lineBXMax
       && y >= lineBYMin && y <= lineBYMax)
      {
         return Point{ (uint16_t)x, (uint16_t)y };
      }
   }
   // No intersection
   return std::nullopt;
}

uint16_t MageGeometry::getLoopableGeometryPointIndex(uint8_t pointIndex) const
{
   uint16_t result = 0;
   if (GetPointCount() == 1)
   {
      // handle the derp who made a poly* with 1 point
   }
   else if (typeId == MageGeometryType::Polygon)
   {
      result = pointIndex % GetPointCount();
   }
   else if (typeId == MageGeometryType::Polyline)
   {
      pointIndex %= (segmentCount * 2);
      result = (pointIndex < GetPointCount())
         ? pointIndex
         : segmentCount + (segmentCount - pointIndex);
   }
   return result;
}