/*
 * $Id: map.cpp,v 1.28 2007/03/07 22:01:36 nathanst Exp $
 *
 * This file is part of HOG.
 *
 * HOG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * HOG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with HOG; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/ 

#include <stack>
#include <cstdlib>
#include <cstring>

#include "Map.h"

using namespace std;

static const bool verbose = false;

/** 
* Construct a half tile, initializing to flat values.
*
* A halfTile is the lowest level in the map - it contains the height of the
* three relevant corners. The heights are initialized to 0, the type of
* territory is initialized to ground.
*/		
halfTile::halfTile()
{
	corners[0] = corners[1] = corners[2] = 0;
	type = kGround;
	node = -1;
}

/** 
* Construct a tile with no split.
*
* A Tile contains two half tiles and information about how the tile is
* split. If a tile isn't split, the first three parts of the tile are in
* the first halfTile and the fourth is in th second.
*/
Tile::Tile()
:tile1(), tile2(), split(kNoSplit)
{ }

/** 
* Create a new map of a particular size.
*
* A map is an array of tiles according to the height and width of the map.
*/
Map::Map(long _width, long _height)
:width(_width), height(_height)
{
	mapType = kOctile;
	map_name[0] = 0;
	sizeMultiplier = 1;
	land = new Tile *[width];

	for (int x = 0; x < width; x++) land[x] = new Tile [height];
	revision = 0;
}

/** 
* Create a new map by copying it from another map.
*
* Creates a new map and initializes it with the map passed to it.
*/
Map::Map(Map *m)
{
	mapType = m->mapType;
	strncpy(map_name, m->map_name, 128);
	sizeMultiplier = m->sizeMultiplier;
	width = m->width;
	height = m->height;
	
	land = new Tile *[width];
	for (int x = 0; x < width; x++) land[x] = new Tile [height];

	revision = m->revision;
	
	for (int x = 0; x < width; x++)
		for (int y = 0; y < height; y++)
			land[x][y] = m->land[x][y];
}

/** 
* Create a new map by loading it from a file.
*
* Creates a new map and initializes it with the file passed to it.
*/
Map::Map(const char *filename)
{
	sizeMultiplier = 1;
	land = 0;
	Load(filename);
}

/** 
* Create a new map by loading it from a file pointer.
*
* Creates a new map and initializes it with the file pointer passed to it.
*/
Map::Map(FILE *f)
{
	sizeMultiplier = 1;
	map_name[0] = 0;
	land = 0;
	Load(f);
}


Map::~Map()
{
	for (int x = 0; x < width; x++)
		delete [] land[x];
	delete [] land;
	land = 0;
}


/** 
* Resets the current map by loading the file passed in.
*
* Resets the current map by loading the file passed in.
*/
void Map::Load(const char *filename)
{
	if (land)
	{
		for (int x = 0; x < width; x++)
			delete [] land[x];
		delete [] land;
		land = 0;
	}
	revision++;
	FILE *f = fopen(filename, "r");
	if (f)
	{
		Load(f);
		fclose(f);
		strncpy(map_name, filename, 128);
	}
	else {
		printf("Error! Can't open file %s\n", filename);
		width = height = 64;
		land = new Tile *[width];

		for (int x = 0; x < width; x++)
			land[x] = new Tile [height];
		map_name[0] = 0;
	}
}

/** 
* Resets the current map by loading the file from the pointer passed in.
*/
void Map::Load(FILE *f)
{
	if (land)
	{
		for (int x = 0; x < width; x++)
			delete [] land[x];
		delete [] land;
		land = 0;
	}
	
	char format[32];
	// ADD ERROR HANDLING HERE
	int num = fscanf(f, "type %s\nheight %d\nwidth %d\nmap\n", format, &height, &width);
	// printf("got height %d, width %d\n", height, width);
	if (num == 3)
	{
		if (strcmp(format, "octile") == 0)
			loadOctile(f, height, width);
		else if (strcmp(format, "octile-corner") == 0)
			loadOctileCorner(f, height, width);
		return;
	}

	{
		printf("Unknown map type; aborting load!\n");
		width = height = 64;
		land = new Tile *[width];
		//	for (int x = 0; x < 8; x++)
		//		g[x] = 0;
		for (int x = 0; x < width; x++) land[x] = new Tile [height];
		map_name[0] = 0;
	}
}


void Map::loadOctile(FILE *f, int high, int wide)
{
	mapType = kOctile;
	height = high*sizeMultiplier;
	width = wide*sizeMultiplier;
	land = new Tile *[wide*sizeMultiplier];
	for (int x = 0; x < wide*sizeMultiplier; x++) land[x] = new Tile [high*sizeMultiplier];
	for (int y = 0; y < high; y++)
	{
		for (int x = 0; x < wide; x++)
		{
			char what;
			fscanf(f, "%c", &what);
			switch (toupper(what))
			{
				case '@':
				case 'O':
					for (int r = 0; r < sizeMultiplier; r++)
						for (int s = 0; s < sizeMultiplier; s++)
							SetTerrainType(x*sizeMultiplier+r, y*sizeMultiplier+s,
														 kOutOfBounds); break;
				case 'S':
					for (int r = 0; r < sizeMultiplier; r++)
						for (int s = 0; s < sizeMultiplier; s++)
							SetTerrainType(x*sizeMultiplier+r, y*sizeMultiplier+s,
														 kSwamp); break;
				case 'W':
					for (int r = 0; r < sizeMultiplier; r++)
						for (int s = 0; s < sizeMultiplier; s++)
							SetTerrainType(x*sizeMultiplier+r, y*sizeMultiplier+s,
														 kWater); break;
				case 'T':
					for (int r = 0; r < sizeMultiplier; r++)
						for (int s = 0; s < sizeMultiplier; s++)
							SetTerrainType(x*sizeMultiplier+r, y*sizeMultiplier+s,
														 kTrees); break;
				default:
					for (int r = 0; r < sizeMultiplier; r++)
						for (int s = 0; s < sizeMultiplier; s++)
							SetTerrainType(x*sizeMultiplier+r, y*sizeMultiplier+s,
														 kGround); break;
			}
			for (int r = 0; r < sizeMultiplier; r++)
				for (int s = 0; s < sizeMultiplier; s++)
				{
					land[x*sizeMultiplier+r][y*sizeMultiplier+s].tile1.node = kNoGraphNode;
					land[x*sizeMultiplier+r][y*sizeMultiplier+s].tile2.node = kNoGraphNode;
				}
		}
			fscanf(f, "\n");
	}
}

void Map::loadOctileCorner(FILE *f, int high, int wide)
{
	mapType = kOctileCorner;
	width--;
	height--;
	land = new Tile *[width];
	for (int x = 0; x < width; x++) land[x] = new Tile [height];
	for (int y = 0; y < high; y++)
	{
		for (int x = 0; x < wide; x++)
		{
			int h;
			char hc;
			fscanf(f, "%c", &hc);
			h = hc-'0'-2;
			if ((x > 0) && (y < high-1))
				SetCornerHeight(x-1, y, kTopRight, h);
			if ((x < wide-1) && (y > 0))
				SetCornerHeight(x, y-1, kBottomLeft, h);
			if ((x > 0) && (y > 0))
				SetCornerHeight(x-1, y-1, kBottomRight, h);
			if ((x < wide-1) && (y < high-1))
				SetCornerHeight(x, y, kTopLeft, h);
		}
		fscanf(f, "\n");
	}
	for (int y = 0; y < high; y++)
	{
		for (int x = 0; x < wide; x++)
		{
			char what;
			fscanf(f, "%c", &what);
			if ((y == high-1) || (x == wide-1))
				continue;
			if (!islower(what))
				SetHeight(x, y, GetCornerHeight(x, y, kTopLeft));
			switch (toupper(what))
			{
				case 'O':
					SetTerrainType(x, y, kOutOfBounds); break;
				case 'S':
					SetTerrainType(x, y, kSwamp); break;
				case 'W':
					SetTerrainType(x, y, kWater); break;
				case 'T':
					SetTerrainType(x, y, kTrees); break;
				default:
					SetTerrainType(x, y, kGround); break;
			}
			land[x][y].tile1.node = kNoGraphNode;
			land[x][y].tile2.node = kNoGraphNode;
		}
		fscanf(f, "\n");
	}
}

/** 
* Saves the current map out to the designated file.
*
* Saves the current map out to the designated file.
*/
void Map::Save(const char *filename)
{
	FILE *f = fopen(filename, "w+");
	if (f)
	{
		Save(f);
		fclose(f);
	}
	else {
		printf("Error! Couldn't open file to save\n");
	}
}

/** 
* Saves the current map out to the designated file.
*
* Saves the current map out to the designated file.
*/
void Map::Save(FILE *f)
{
	switch(mapType)
	{
		case kOctile:
			saveOctile(f);
			break;
		case kOctileCorner:
			printf("Unable to save to identical map type\n");
		default:
			break;
	}
}

void Map::saveOctile(FILE *f)
{
	if (f)
	{
		fprintf(f, "type octile\nheight %d\nwidth %d\nmap\n", height, width);
		for (int y = 0; y < height; y++)
		{
			for (int x = 0; x < width; x++)
			{
				switch (GetTerrainType(x, y))
				{
					case kGround: fprintf(f, "."); break;
					case kSwamp: fprintf(f, "S"); break;
					case kWater: fprintf(f, "W"); break;
					case kTrees: fprintf(f, "T"); break;
					default: fprintf(f, "@"); break; // out of bounds
				}
			}
			fprintf(f, "\n");
		}
	}
}

const char *Map::GetMapName()
{
	if (map_name[0] == 0)
		return "";
	return map_name;
}

/** 
* Return the tile at location x, y.
*
* returns a reference to the type at a particular x/y location. (starting from 0)
*/

Tile &Map::GetTile(long x, long y)
{
	return land[x][y];
}

/** 
* Return the split of the tile at x, y.
*
* Returns the type of split; either kNoSplit, kForwardSplit, kBackwardSplit
*/
tSplit Map::GetSplit(long x, long y) const
{
	return land[x][y].split;
}

/** 
* Set the split of the tile at x, y.
*
* Sets how a map is split; either kNoSplit, kForwardSplit, kBackwardSplit
*/
void Map::SetSplit(long x, long y, tSplit split)
{
	revision++;
	land[x][y].split = split;
}


/** 
* Get the terrain type of the (split) tile at x, y.
*
* Gets the terrain type for this tile. By default it looks for the value
* of the whole tile. Possible split values are kWholeTile, kLeftSide, and
* kRightSide. 
*/
long Map::GetTerrainType(long x, long y, tSplitSide split) const
{
	if (land[0] == 0)
		printf("land: %p, land[0] = %p\n", land, land[0]);
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) return kUndefined;
	if (split == kRightSide) return land[x][y].tile2.type;
	return land[x][y].tile1.type;
}

/**
 * Map::SetTerrainType()
 *
 * \brief Set all the terrain between two points to be the same
 *
 * \param x1 The first x-coordinate to set
 * \param y1 The first y-coordinate to set
 * \param x2 The second x-coordinate to set
 * \param y2 The second y-coordinate to set
 * \param terrain The terrain for the line between the coordinates
 * \return none
 */
void Map::SetTerrainType(int32_t x1, int32_t y1,
                         int32_t x2, int32_t y2, tTerrain t)
{
    //printf("---> (%d, %d) to (%d, %d)\n", x1, y1, x2, y2);
    double xdiff = (int)x1-(int)x2;
    double ydiff = (int)y1-(int)y2;
    double dist = sqrt(xdiff*xdiff + ydiff*ydiff);
    SetTerrainType(x1, y1, t);
    for (double c = 0; c < dist; c += 0.5)
    {
        double ratio = c/dist;
        double newx = (double)x1-xdiff*ratio;
        double newy = (double)y1-ydiff*ratio;
        SetTerrainType((uint32_t)newx, (uint32_t)newy, t);
    }
    SetTerrainType(x2, y2, t);
}


/**
* Get the terrain type for one side of the tile at x, y.
 *
 * Gets the terrain type for a particular edge of the type. 
 * (kLeftEdge, kRightEdge, kTopEdge, kBottom Edge)
 * This function avoids making you figure out all the ways a tile could
 * be split to get the correct value out.
 */
long Map::GetTerrainType(long x, long y, tEdge side) const
{
	if ((x < 0) || (x >= width) || (y < 0) || (y >= height)) return kUndefined;
	if (land[x][y].split == kNoSplit) return land[x][y].tile1.type;
	switch (side) {
		case kLeftEdge:
			return land[x][y].tile1.type;
		case kRightEdge:
			return land[x][y].tile2.type;
		case kTopEdge:
			if (land[x][y].split == kForwardSplit)
				return land[x][y].tile1.type;
			else
				return land[x][y].tile2.type;
		case kBottomEdge:
			if (land[x][y].split == kBackwardSplit)
				return land[x][y].tile1.type;
			else
				return land[x][y].tile2.type;
		default:
			return kUndefined;
	}
}

/** Set the terrain type of the side of the tile at x, y.
*
* side is one of kWholeTile, kLeftSide or kRightSide
* If tile is not split and you specify a split side, nothing happens
* If tile is split and you specify kWholeTile, the split remains,
* and the terrain is applied to both sides.
*/
void Map::SetTerrainType(long x, long y, tTerrain type, tSplitSide split)
{
	if ((x >= width)||(x<0)) return;
	if ((y >= height)||(x<0)) return;
	revision++;
	map_name[0] = 0;
	if ((land[x][y].split == kNoSplit) && (split != kWholeTile)) return;
	if ((y > GetMapHeight()-1) || (x > GetMapWidth()-1))
		return;
	switch (split) {
		
		case kWholeTile:
			land[x][y].tile1.type = type;
			land[x][y].tile2.type = type;
			break;
			
		case kLeftSide:
			land[x][y].tile1.type = type;
			break;
			
		case kRightSide:
			land[x][y].tile2.type = type;
			break;
	}
}

/** 
* Get the (flat) height of the tile at x, y.
*
* returns the height of a particular tile -- actually just one corner of the
* tile. If the tile is sloping you'll get back kUndefined and need to get
* the specific corner heights.
* returns kUndefinedHeight if the tile is split and you specify
* the whole tile
*/
long Map::GetHeight(long x, long y, tSplitSide split)
{
	if ((land[x][y].split != kNoSplit) && (split == kWholeTile)) return kUndefinedHeight;
	
	switch (split) {
		
		case kWholeTile:
			
		case kLeftSide:
			if ((land[x][y].tile1.corners[0] == land[x][y].tile1.corners[1]) &&
					(land[x][y].tile1.corners[1] == land[x][y].tile1.corners[2]))
				return land[x][y].tile1.corners[0];
			return kUndefinedHeight;
			break;
			
		case kRightSide:
			if ((land[x][y].tile2.corners[0] == land[x][y].tile2.corners[1]) &&
					(land[x][y].tile2.corners[1] == land[x][y].tile2.corners[2]))
				return land[x][y].tile2.corners[0];
			break;
	}
	return kUndefinedHeight;
}

/** 
* Set the (flat) height of the tile at x, y.
* 
* Split is kWholeTile, kLeftSide or kRightSide.
*/
void Map::SetHeight(long x, long y, long tHeight, tSplitSide split)
{
	revision++;
	switch (split) {
		
		case kWholeTile:
			land[x][y].tile1.corners[0] = land[x][y].tile1.corners[1] =
			land[x][y].tile1.corners[2] = tHeight;
			land[x][y].tile2.corners[0] = tHeight;
			land[x][y].tile2.corners[1] = tHeight;
			land[x][y].tile2.corners[2] = tHeight;
			break;
		case kLeftSide:
			land[x][y].tile1.corners[0] = land[x][y].tile1.corners[1] =
			land[x][y].tile1.corners[2] = tHeight;
			break;
		case kRightSide:
			land[x][y].tile2.corners[0] = land[x][y].tile2.corners[1] =
			land[x][y].tile2.corners[2] = tHeight;
			break;
	}
}

/**
* Set the height of any one corner of a tile.
 * 
 * corner is kTopLeft, kBottomLeft, kTopRight or kBottomRight
 * edge is kBottomEdge, kLeftEdge, kRightEdge, kTopEdge
 * returns kUndefinedHeight if the split is inconsistant with the tile type
 * The combination of a corner and an edge uniquely define a single height
 */
long Map::GetCornerHeight(long x, long y, tCorner which, tEdge edge) const
{
	if (GetSplit(x, y) == kNoSplit)
	{
		switch (which) {
			case kTopLeft: return land[x][y].tile1.corners[0];
			case kBottomLeft: return land[x][y].tile1.corners[1];
			case kTopRight: return land[x][y].tile2.corners[0];
			case kBottomRight: return land[x][y].tile1.corners[2];
			default: break;
		}
	}
	else {
		if (edge == kLeftEdge)
		{
			switch (which) {
				case kTopLeft: return land[x][y].tile1.corners[0];
				case kBottomLeft: return land[x][y].tile1.corners[1];
				default: break;
			}
		}
		else if (edge == kRightEdge)
		{
			switch (which) {
				case kTopRight: return land[x][y].tile2.corners[0];
				case kBottomRight: return land[x][y].tile2.corners[1];
				default: break;
			}
		}
		else if ((edge == kTopEdge) && (GetSplit(x, y) == kForwardSplit))
		{
			switch (which) {
				case kTopLeft: return land[x][y].tile1.corners[0];
				case kTopRight: return land[x][y].tile1.corners[2];
				default: break;
			}
		}
		else if ((edge == kTopEdge) && (GetSplit(x, y) == kBackwardSplit))
		{
			switch (which) {
				case kTopLeft: return land[x][y].tile2.corners[2];
				case kTopRight: return land[x][y].tile2.corners[0];
				default: break;
			}
		}
		else if ((edge == kBottomEdge) && (GetSplit(x, y) == kForwardSplit))
		{
			switch (which) {
				case kBottomLeft: return land[x][y].tile2.corners[2];
				case kBottomRight: return land[x][y].tile2.corners[1];
				default: break;
			}
		}
		else if ((edge == kBottomEdge) && (GetSplit(x, y) == kBackwardSplit))
		{
			switch (which) {
				case kBottomLeft: return land[x][y].tile1.corners[1];
				case kBottomRight: return land[x][y].tile1.corners[2];
				default: break;
			}
		}
	}
	// should never get here...
	return kUndefinedHeight;
}

/**
* Get the height of any one corner of a tile.
 * 
 * corner is kTopLeft, kBottomLeft, kTopRight or kBottomRight
 * split is kLeftSide, kRightSide or kWholeTile
 * returns kUndefinedHeight if the split is inconsistant with the tile type
 * The combination of a corner and a split side uniquely define a single height
 */
long Map::GetCornerHeight(long x, long y, tCorner which, tSplitSide split) const
{
	if ((land[x][y].split != kNoSplit) && (split == kWholeTile))
		return kUndefinedHeight;
	if (split == kWholeTile)
	{
		switch (which) {
			case kTopLeft: return land[x][y].tile1.corners[0];
			case kBottomLeft: return land[x][y].tile1.corners[1];
			case kTopRight: return land[x][y].tile2.corners[0];
			case kBottomRight: return land[x][y].tile1.corners[2];
			default: break;
		}
	}
	else if (split == kLeftSide)
	{
		switch (which) {
			case kTopLeft: return land[x][y].tile1.corners[0];
			case kBottomLeft: return land[x][y].tile1.corners[1];
			case kTopRight:
				if (land[x][y].split == kForwardSplit)
					return land[x][y].tile1.corners[2];
				return kUndefinedHeight;
			case kBottomRight:
				if (land[x][y].split == kBackwardSplit)
					return land[x][y].tile1.corners[2];
				return kUndefinedHeight;
			default: break;
		}
	}
	else if (split == kRightSide)
	{
		switch (which) {
			case kTopRight: return land[x][y].tile2.corners[0];
			case kBottomRight: return land[x][y].tile2.corners[1];
			case kTopLeft:
				if (land[x][y].split == kBackwardSplit)
					return land[x][y].tile2.corners[2];
				return kUndefinedHeight;
			case kBottomLeft:
				if (land[x][y].split == kForwardSplit)
					return land[x][y].tile2.corners[2];
				return kUndefinedHeight;
			default: break;
		}
	}
	return kUndefinedHeight;
}

/**
* Set the height of any one corner of a tile.
 * 
 * corner is kTopLeft, kBottomLeft, kTopRight or kBottomRight
 * split is kLeftSide, kRightSide or kWholeTile
 * The combination of a corner and a split side uniquely define a single height,
 * which is returned.
 */
void Map::SetCornerHeight(long x, long y, tCorner which,
													long cHeight, tSplitSide split)
{
	if ((land[x][y].split != kNoSplit) && (split == kWholeTile))
		return;
	revision++;
	if (split == kWholeTile)
	{
		switch (which) {
			case kTopLeft: land[x][y].tile1.corners[0] = cHeight; break;
			case kBottomLeft: land[x][y].tile1.corners[1] = cHeight; break;
			case kTopRight: land[x][y].tile2.corners[0] = cHeight; break;
			case kBottomRight: land[x][y].tile1.corners[2] = cHeight; 
				land[x][y].tile2.corners[1] = cHeight;break;
			default: break;
		}
	}
	else if (split == kLeftSide)
	{
		switch (which) {
			case kTopLeft: land[x][y].tile1.corners[0] = cHeight; break;
			case kBottomLeft: land[x][y].tile1.corners[1] = cHeight; break;
			case kTopRight:
				if (land[x][y].split == kForwardSplit)
					land[x][y].tile1.corners[2] = cHeight;
				break;
			case kBottomRight:
				if (land[x][y].split == kBackwardSplit)
					land[x][y].tile1.corners[2] = cHeight;
				break;
			default: break;
		}
	}
	else if (split == kRightSide)
	{
		switch (which) {
			case kTopRight: land[x][y].tile2.corners[0] = cHeight; break;
			case kBottomRight: land[x][y].tile2.corners[1] = cHeight; break;
			case kTopLeft:
				if (land[x][y].split == kBackwardSplit)
					land[x][y].tile2.corners[2] = cHeight;
				break;
			case kBottomLeft:
				if (land[x][y].split == kForwardSplit)
					land[x][y].tile2.corners[2] = cHeight;
				break;
			default: break;
		}
	}
}


/**
* Places a rectangle into the map, but also modifies the edges to make the
 * transition smooth.
 *
 * sets a rectangle of with corner coordinates (x1, y1) (x2, y2)
 * but also takes the 1-radius tiles surrounding that rectangle and
 * smooths them so you get a nice fit of land together.
 */
void Map::SmoothSetRectHeight(long x1, long y1, long x2, long y2, long h, tTerrain type)
{
	map_name[0] = 0;
	if (x1 > x2)
	{
		SmoothSetRectHeight(x2, y1, x1, y2, h, type);
		return;
	}
	else if (y1 > y2)
	{
		SmoothSetRectHeight(x1, y2, x2, y1, h, type);
		return;
	}
	printf("Doing smooth rect between (%ld, %ld) and (%ld, %ld) height %ld\n", x1, y1, x2, y2, h);
	SetRectHeight(x1, y1, x2, y2, h, type);
	
	// top side
	for (int x = x1; x <= x2; x++)
	{
		SetTerrainType(x, y1-1, type);
		switch(GetSplit(x, y1-1)) {
			case kNoSplit:
				if (GetCornerHeight(x, y1-1, kTopLeft) != GetCornerHeight(x, y1-1, kTopRight))
				{ // need to split slanted tile
					SetSplit(x, y1-1, kForwardSplit); // split is arbitarary?
																						//SetCornerHeight(x, y1-1, kBottomLeft, GetCornerHeight(x, y1-1, kBottomLeft, kLeftSide), kRightSide);
					
					SetCornerHeight(x, y1-1, kBottomLeft, h, kRightSide);
					SetCornerHeight(x, y1-1, kBottomRight, h, kRightSide);
					SetCornerHeight(x, y1-1, kBottomLeft, h, kLeftSide);
				}
				else {
					SetCornerHeight(x, y1-1, kBottomLeft, h);
					SetCornerHeight(x, y1-1, kBottomRight, h);
				}
				break;
			case kForwardSplit:
				SetCornerHeight(x, y1-1, kBottomLeft, h, kRightSide);
				SetCornerHeight(x, y1-1, kBottomRight, h, kRightSide);
				SetCornerHeight(x, y1-1, kBottomLeft, h, kLeftSide);
				break;
			case kBackwardSplit:
				SetCornerHeight(x, y1-1, kBottomLeft, h, kLeftSide);
				SetCornerHeight(x, y1-1, kBottomRight, h, kLeftSide);
				SetCornerHeight(x, y1-1, kBottomRight, h, kRightSide);
				break;
		}
	}
	
	// bottom side
	for (int x = x1; x <= x2; x++)
	{
		SetTerrainType(x, y2+1, type);
		switch(GetSplit(x, y2+1)) {
			case kNoSplit:
				if (GetCornerHeight(x, y2+1, kBottomLeft) != GetCornerHeight(x, y2+1, kBottomRight))
				{ // need to split slanted tile
					SetSplit(x, y2+1, kBackwardSplit); // split is arbitarary?
																						 //SetCornerHeight(x, y2+1, kTopLeft, GetCornerHeight(x, y2+1, kTopLeft, kLeftSide), kRightSide);
					
					SetCornerHeight(x, y2+1, kTopLeft, h, kRightSide);
					SetCornerHeight(x, y2+1, kTopRight, h, kRightSide);
					SetCornerHeight(x, y2+1, kTopLeft, h, kLeftSide);
				}
				else {
					SetCornerHeight(x, y2+1, kTopLeft, h);
					SetCornerHeight(x, y2+1, kTopRight, h);
				}
				break;
			case kBackwardSplit:
				SetCornerHeight(x, y2+1, kTopLeft, h, kRightSide);
				SetCornerHeight(x, y2+1, kTopRight, h, kRightSide);
				SetCornerHeight(x, y2+1, kTopLeft, h, kLeftSide);
				break;
			case kForwardSplit:
				SetCornerHeight(x, y2+1, kTopLeft, h, kLeftSide);
				SetCornerHeight(x, y2+1, kTopRight, h, kLeftSide);
				SetCornerHeight(x, y2+1, kTopRight, h, kRightSide);
				break;
		}
	}
	
	// left side
	for (int y = y1; y <= y2; y++)
	{
		SetTerrainType(x1-1, y, type);
		switch(GetSplit(x1-1, y)) {
			case kNoSplit:
				if (GetCornerHeight(x1-1, y, kTopLeft) != GetCornerHeight(x1-1, y, kBottomLeft))
				{ // need to split slanted tile
					SetSplit(x1-1, y, kBackwardSplit); // split is arbitarary?
					SetCornerHeight(x1-1, y, kBottomRight, GetCornerHeight(x1-1, y, kTopRight, kLeftSide), kRightSide);
					SetCornerHeight(x1-1, y, kTopLeft, GetCornerHeight(x1-1, y, kTopLeft, kLeftSide), kRightSide);
					
					SetCornerHeight(x1-1, y, kBottomRight, h, kLeftSide);
					SetCornerHeight(x1-1, y, kBottomRight, h, kRightSide);
					SetCornerHeight(x1-1, y, kTopRight, h, kRightSide);
				}
				else {
					SetCornerHeight(x1-1, y, kTopRight, h);
					SetCornerHeight(x1-1, y, kBottomRight, h);
				}
				break;
			case kBackwardSplit:
				SetCornerHeight(x1-1, y, kBottomRight, h, kLeftSide);
				SetCornerHeight(x1-1, y, kBottomRight, h, kRightSide);
				SetCornerHeight(x1-1, y, kTopRight, h, kRightSide);
				//SetCornerHeight(x1-1, y, kTopLeft, GetCornerHeight(x1-1, y, kTopLeft, kLeftSide), kRightSide);
				break;
			case kForwardSplit:
				SetCornerHeight(x1-1, y, kTopRight, h, kLeftSide);
				SetCornerHeight(x1-1, y, kBottomRight, h, kRightSide);
				SetCornerHeight(x1-1, y, kTopRight, h, kRightSide);
				break;
		}
	}
	
	// right side
	for (int y = y1; y <= y2; y++)
	{
		SetTerrainType(x2+1, y, type);
		switch(GetSplit(x2+1, y)) {
			case kNoSplit:
				if (GetCornerHeight(x2+1, y, kTopLeft) != GetCornerHeight(x2+1, y, kBottomLeft))
				{ // need to split slanted tile
					SetSplit(x2+1, y, kForwardSplit); // split is arbitarary?
					SetCornerHeight(x2+1, y, kTopRight, GetCornerHeight(x2+1, y, kTopRight, kRightSide), kLeftSide);
					
					SetCornerHeight(x2+1, y, kBottomLeft, h, kRightSide);
					SetCornerHeight(x2+1, y, kBottomLeft, h, kLeftSide);
					SetCornerHeight(x2+1, y, kTopLeft, h, kLeftSide);
				}
				else {
					SetCornerHeight(x2+1, y, kTopLeft, h);
					SetCornerHeight(x2+1, y, kBottomLeft, h);
				}
				break;
			case kBackwardSplit:
				SetCornerHeight(x2+1, y, kTopLeft, h, kRightSide);
				SetCornerHeight(x2+1, y, kBottomLeft, h, kLeftSide);
				SetCornerHeight(x2+1, y, kTopLeft, h, kLeftSide);
				break;
			case kForwardSplit:
				SetCornerHeight(x2+1, y, kBottomLeft, h, kRightSide);
				SetCornerHeight(x2+1, y, kBottomLeft, h, kLeftSide);
				SetCornerHeight(x2+1, y, kTopLeft, h, kLeftSide);
				break;
		}
	}
	
	
	SetSplit(x1-1, y1-1, kForwardSplit);
	SetTerrainType(x1-1, y1-1, type, kRightSide);
	SetCornerHeight(x1-1, y1-1, kBottomRight, h, kRightSide);
	SetCornerHeight(x1-1, y1-1, kBottomLeft, GetCornerHeight(x1-1, y1-1, kBottomLeft, kLeftSide), kRightSide);
	SetCornerHeight(x1-1, y1-1, kTopRight, GetCornerHeight(x1-1, y1-1, kTopRight, kRightSide), kLeftSide);
	
	SetSplit(x2+1, y2+1, kForwardSplit);
	SetTerrainType(x2+1, y2+1, type, kLeftSide);
	SetCornerHeight(x2+1, y2+1, kTopLeft, h, kLeftSide);
	SetCornerHeight(x2+1, y2+1, kBottomLeft, GetCornerHeight(x2+1, y2+1, kBottomLeft, kLeftSide), kRightSide);
	SetCornerHeight(x2+1, y2+1, kTopRight, GetCornerHeight(x2+1, y2+1, kTopRight, kRightSide), kLeftSide);
	
	SetSplit(x1-1, y2+1, kBackwardSplit);
	SetTerrainType(x1-1, y2+1, type, kRightSide);
	SetCornerHeight(x1-1, y2+1, kTopRight, h, kRightSide);
	SetCornerHeight(x1-1, y2+1, kTopLeft, GetCornerHeight(x1-1, y2+1, kTopLeft, kLeftSide), kRightSide);
	SetCornerHeight(x1-1, y2+1, kBottomRight, GetCornerHeight(x1-1, y2+1, kBottomRight, kRightSide), kLeftSide);
	
	SetSplit(x2+1, y1-1, kBackwardSplit);
	SetTerrainType(x2+1, y1-1, type, kLeftSide);
	SetCornerHeight(x2+1, y1-1, kBottomLeft, h, kLeftSide);
	SetCornerHeight(x2+1, y1-1, kTopLeft, GetCornerHeight(x2+1, y1-1, kTopLeft, kLeftSide), kRightSide);
	SetCornerHeight(x2+1, y1-1, kBottomRight, GetCornerHeight(x2+1, y1-1, kBottomRight, kRightSide), kLeftSide);
}

/**
* Set the height and terrain of a set of tiles.
 *
 * Sets all the tiles in the region between (x1, y1) (x2, y2) to be the same
 * height and terrain type, with no splits.
 */
void Map::SetRectHeight(long x1, long y1, long x2, long y2, long h, tTerrain type)
{
	map_name[0] = 0;
	revision++;
	//printf("Doing rect between (%ld, %ld) and (%ld, %ld) height %ld\n", x1, y1, x2, y2, h);
	for (int x = x1; x <= x2; x++)
	{
		for (int y = y1; y <= y2; y++)
		{
			SetSplit(x, y, kNoSplit);
			SetTerrainType(x, y, type);
			SetHeight(x, y, h);
		}
	}
}

/**
* Is the tile at x, y adjacent across the edge?
 *
 * given an edge (kInternalEdge, kLeftEdge, kRightEdge, kBottomEdge, kTopEdge)
 * returns whether the tiles on both sides of that edge have a smooth boundary
 * that a unit should be able to cross.
 */
bool Map::AdjacentEdges(long x, long y, tEdge edge) const
{
	if ((x < 0) || (y < 0) || (x >= width) || (y >= height))
		return false;
	switch (edge) {
		case kInternalEdge:
		{
			tSplit split;
			if ((split = GetSplit(x, y)) == kNoSplit)
				return ((GetTerrainType(x, y, kLeftSide)>>terrainBits) == (GetTerrainType(x, y, kRightSide)>>terrainBits));
			else if (split == kForwardSplit)
			{
				return ((GetCornerHeight(x, y, kTopRight, kLeftSide) == GetCornerHeight(x, y, kTopRight, kRightSide)) &&
								(GetCornerHeight(x, y, kBottomLeft, kLeftSide) == GetCornerHeight(x, y, kBottomLeft, kRightSide)) &&
								((GetTerrainType(x, y, kLeftSide)>>terrainBits) == (GetTerrainType(x, y, kRightSide)>>terrainBits)));
			}
			else if (split == kBackwardSplit)
			{
				return ((GetCornerHeight(x, y, kTopLeft, kLeftSide) == GetCornerHeight(x, y, kTopLeft, kRightSide)) &&
								(GetCornerHeight(x, y, kBottomRight, kLeftSide) == GetCornerHeight(x, y, kBottomRight, kRightSide)) &&
								((GetTerrainType(x, y, kLeftSide)>>terrainBits) == (GetTerrainType(x, y, kRightSide)>>terrainBits)));
			}
			return false;
		} break;
		case kLeftEdge:
			if (x == 0)
				return false;
			return ((GetCornerHeight(x, y, kTopLeft, kLeftEdge) == GetCornerHeight(x-1, y, kTopRight, kRightEdge)) &&
							(GetCornerHeight(x, y, kBottomLeft, kLeftEdge) == GetCornerHeight(x-1, y, kBottomRight, kRightEdge)) &&
							((GetTerrainType(x, y, kLeftSide)>>terrainBits) == (GetTerrainType(x-1, y, kRightSide)>>terrainBits)));
			break;
		case kRightEdge:
			if (x+1 >= width)
				return false;
			return ((GetCornerHeight(x, y, kTopRight, kRightEdge) == GetCornerHeight(x+1, y, kTopLeft, kLeftEdge)) &&
							(GetCornerHeight(x, y, kBottomRight, kRightEdge) == GetCornerHeight(x+1, y, kBottomLeft, kLeftEdge)) &&
							((GetTerrainType(x, y, kRightSide)>>terrainBits) == (GetTerrainType(x+1, y, kLeftSide)>>terrainBits)));
			break;
		case kTopEdge:
			if (y == 0)
				return false;
			return ((GetCornerHeight(x, y, kTopRight, kTopEdge) == GetCornerHeight(x, y-1, kBottomRight, kBottomEdge)) &&
					(GetCornerHeight(x, y, kTopLeft, kTopEdge) == GetCornerHeight(x, y-1, kBottomLeft, kBottomEdge)) &&
					//(CanPass(GetTerrainType(x, y, kTopEdge), GetTerrainType(x, y-1, kBottomEdge))));
					((GetTerrainType(x, y, kTopEdge)>>terrainBits) == (GetTerrainType(x, y-1, kBottomEdge)>>terrainBits)));
			
			break;
		case kBottomEdge:
			if (y+1 >= height)
				return false;
			return ((GetCornerHeight(x, y, kBottomRight, kBottomEdge) == GetCornerHeight(x, y+1, kTopRight, kTopEdge)) &&
					(GetCornerHeight(x, y, kBottomLeft, kBottomEdge) == GetCornerHeight(x, y+1, kTopLeft, kTopEdge)) &&
					//(CanPass(GetTerrainType(x, y, kBottomEdge), GetTerrainType(x, y+1, kTopEdge))));
					((GetTerrainType(x, y, kBottomEdge)>>terrainBits) == (GetTerrainType(x, y+1, kTopEdge)>>terrainBits)));
			break;
	}
	return false;
}

bool Map::AdjacentCorners(long x, long y, tCorner corner) const
{
	if ((x < 0) || (y < 0) || (x >= width) || (y >= height))
		return false;
	switch (corner)
	{
		case kNone:
			return true;
		case kTopLeft:
			if (((x >= 1) && (y >= 1) && (AdjacentEdges(x, y, kLeftEdge)) && (AdjacentEdges(x, y, kTopEdge)) &&
					 (AdjacentEdges(x, y-1, kLeftEdge)) && (AdjacentEdges(x-1, y, kTopEdge))) &&
					(((AdjacentEdges(x-1, y, kInternalEdge)) || (GetSplit(x-1, y) == kBackwardSplit)) &&
					 ((AdjacentEdges(x, y-1, kInternalEdge)) || (GetSplit(x, y-1) == kBackwardSplit)) &&
					 ((AdjacentEdges(x-1, y-1, kInternalEdge)) || (GetSplit(x-1, y-1) == kForwardSplit)) &&
					 ((AdjacentEdges(x, y, kInternalEdge)) || (GetSplit(x, y) == kForwardSplit))))
				return true;
			return false;
		case kTopRight:
			if (((y >= 1) && (x < GetMapWidth()-1) && (AdjacentEdges(x, y, kRightEdge)) && (AdjacentEdges(x, y, kTopEdge)) &&
					 (AdjacentEdges(x, y-1, kRightEdge)) && (AdjacentEdges(x+1, y, kTopEdge))) &&
					(((AdjacentEdges(x+1, y, kInternalEdge)) || (GetSplit(x+1, y) == kForwardSplit)) &&
					 ((AdjacentEdges(x, y-1, kInternalEdge)) || (GetSplit(x, y-1) == kForwardSplit)) &&
					 ((AdjacentEdges(x+1, y-1, kInternalEdge)) || (GetSplit(x+1, y-1) == kBackwardSplit)) &&
					 ((AdjacentEdges(x, y, kInternalEdge)) || (GetSplit(x, y) == kBackwardSplit))))
				return true;
			return false;
		case kBottomLeft: return AdjacentCorners(x-1, y+1, kTopRight);
		case kBottomRight: return AdjacentCorners(x+1, y+1, kTopLeft);
		default: return false;
	}
	return false;
}

bool Map::CanStep(long x1, long y1, long x2, long y2) const
{
	if ((abs(x1-x2) > 1) || (abs(y1-y2) > 1))
		return false;
	if (mapType == kOctile)
	{
		return ((GetTerrainType(x1, y1)>>terrainBits) == (GetTerrainType(x2, y2)>>terrainBits));
	}
	else {
		switch (x1-x2) {
			case 0: //return true;
				switch (y1-y2) {
					case 0: return true;
					case 1: return AdjacentEdges(x1, y1, kTopEdge);
					case -1: return AdjacentEdges(x1, y1, kBottomEdge);
				}
				break;
			case 1: //return AdjacentEdges(x1, y1, kLeftEdge);
				switch (y1-y2) {
					case 0: return AdjacentEdges(x1, y1, kLeftEdge);
					case 1: return AdjacentCorners(x1, y1, kTopLeft);
					case -1: return AdjacentCorners(x1, y1, kBottomLeft);
				}
				break;
			case -1: //return AdjacentEdges(x1, y1, kRightEdge);
				switch (y1-y2) {
					case 0: return AdjacentEdges(x1, y1, kRightEdge);
					case 1: return AdjacentCorners(x1, y1, kTopRight);
					case -1: return AdjacentCorners(x1, y1, kBottomRight);
				}
				break;
		}
	}
	return false;
}





/**
* Sets the abstract Graph node number for this tile.
 *
 * Because we have a Graph representation of the map as well, we need some
 * way to get back and forth between the representations. This function will
 * set the unique data (nodeNum) for a tile/half tile so that we can go
 * from a tile in the map to a node in the Graph.
 */
void Map::SetNodeNum(int num, int x, int y, tCorner corner)
{
	if ((x < 0) || (y < 0) || (x >= width) || (y >= height))
	{
		printf("ERROR -- trying to set invalid node number!\n");
		return;
	}
	if ((corner == kBottomRight) || (corner == kTopRight))
		land[x][y].tile2.node = num;
	land[x][y].tile1.node = num;
}

/**
* Gets the abstract Graph node number for this tile.
 *
 * Because we have a Graph representation of the map as well, we need some
 * way to get back and forth between the representations. This function will
 * get the unique data (nodeNum) for a tile/half tile so that we can go
 * from a tile in the map to a node in the Graph.
 */
int Map::GetNodeNum(int x, int y, tCorner corner)
{
	if ((x < 0) || (y < 0) || (x >= width) || (y >= height))
	{
		//printf("ERROR -- trying to get invalid node number!\n");
		return -1;
	}
	if ((corner == kBottomRight) || (corner == kTopRight))
		return land[x][y].tile2.node;
	return land[x][y].tile1.node;
}

