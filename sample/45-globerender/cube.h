// Helper structures
struct Vector3D
{
	float x, y, z;
	
	Vector3D (float x = 0.0f, float y = 0.0f, float z = 0.0f)
	:	x (x), y (y), z (z)
	{
	}
};

struct Point2D
{
	int x, y;
	
	Point2D (int x = 0, int y = 0)
	:	x (x), y (y)
	{
	}
};


// Cube vertices
static const Vector3D vs[] = {
	{ 0.25f,  0.25f,  0.25f},
	{-0.25f,  0.25f,  0.25f},
	{-0.25f, -0.25f,  0.25f},
	{ 0.25f, -0.25f,  0.25f},
	{ 0.25f,  0.25f, -0.25f},
	{-0.25f,  0.25f, -0.25f},
	{-0.25f, -0.25f, -0.25f},
	{ 0.25f, -0.25f, -0.25f}
};

// Cube faces (indices into vs array)
static const unsigned fs[][4] = {
	{0, 1, 2, 3},  // front face
	{4, 5, 6, 7},  // back face
	{0, 4},        // top edge
	{1, 5},        // top edge
	{2, 6},        // bottom edge
	{3, 7}         // bottom edge
};

static const unsigned fs_sizes[] = {4, 4, 2, 2, 2, 2};
static const unsigned num_faces = 6;