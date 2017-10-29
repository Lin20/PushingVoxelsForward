#pragma once

static const int T_CUBE_INDEXES[][4] =
{
	{ 0, 1, 5, 3 },
	{ 0, 4, 5, 3 },
	{ 7, 4, 5, 3 },
	{ 7, 6, 5, 3 },
	{ 2, 6, 5, 3 },
	{ 2, 1, 5, 3 }
};

static const int T_PEM_CUBE_INDEXES[][4] =
{
	{ 0, 1, 3, 4 },
	{ 0, 2, 3, 4 },
	{ 6, 2, 3, 4 },
	{ 6, 7, 3, 4 },
	{ 5, 7, 3, 4 },
	{ 5, 1, 3, 4 }
};

static const int T_VERTEX_EDGE_MAPS[][2] =
{
	{ 0, 1 },
	{ 0, 2 },
	{ 0, 3 },
	{ 1, 2 },
	{ 1, 3 },
	{ 2, 3 }
};

static const int H_VERTEX_MAPS[][4] = 
{
	{ 0, -1, -1, -1 },
	{ 0, 1, -1, -1 },
	{ 0, 1, 3, -1 },
	{ 3, 0, -1, -1 },
	{ 0, 2, -1, -1 },
	{ 0, 1, 2, -1 },
	{ 0, 1, 2, 3 },
	{ 0, 2, 3, -1 }
};

static const int H_VERTEX_TRANSLATIONS[][4] = 
{
	{ 0, 1, 2, 3 },
	{ 1, 2, 3, 0 },
	{ 2, 3, 0, 1 },
	{ 3, 0, 1, 2 }
};

static const int H_VERTEX_COUNTS[] = { 1, 2, 3, 2, 2, 3, 4, 3 };