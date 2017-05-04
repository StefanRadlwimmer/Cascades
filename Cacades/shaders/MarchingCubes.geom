#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 15) out;

in Gridcell {
   vec3 p[8];
   float val[8];
   int mc_case;
} gs_in[];

uniform int isoLevel = 0;
uniform vec3 scale;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 position;
out vec3 normal;

struct CellTriangles
{
	int tris[16];
};

layout (shared, std140) uniform MC_EdgeTable
{
	int edgeTable[256];
};
layout (shared, std140) uniform MC_TrisTable
{
	CellTriangles triTable[256];
};

/*
   Linearly interpolate the position where an isosurface cuts
   an edge between two vertices, each with their own scalar value
*/
vec3 VertexInterp(float isoLevel, vec3 p1, vec3 p2, float valp1, float valp2)
{
	float mu;
	vec3 p;

	if (abs(isoLevel-valp1) < 0.00001)
	   return(p1);
	if (abs(isoLevel-valp2) < 0.00001)
	   return(p2);
	if (abs(valp1-valp2) < 0.00001)
	   return(p1);
	mu = (isoLevel - valp1) / (valp2 - valp1);
	p.x = p1.x + mu * (p2.x - p1.x);
	p.y = p1.y + mu * (p2.y - p1.y);
	p.z = p1.z + mu * (p2.z - p1.z);

	return(p);
}

vec3 CalculateNormal(vec3 p1, vec3 p2, vec3 p3)
{
	vec3 normal;

	vec3 u = p2 - p1;
	vec3 v = p3 - p1;

	normal.x = (u.y * v.z) - (u.z * v.y);
	normal.y = (u.z * v.x) - (u.x * v.z);
	normal.z = (u.x * v.y) - (u.y * v.x);

	return normalize(normal);
}

void main()
{
	vec3 vertlist[12] = vec3[] (
	vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f),
	vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f), vec3(0.0f));

	/* Find the vertices where the surface intersects the cube */
	if ((edgeTable[gs_in[0].mc_case] & 1) != 0)
		vertlist[ 0] = VertexInterp(isoLevel, gs_in[0].p[0], gs_in[0].p[1], gs_in[0].val[0], gs_in[0].val[1]);
	if ((edgeTable[gs_in[0].mc_case] & 2) != 0)
	    vertlist[ 1] = VertexInterp(isoLevel, gs_in[0].p[1], gs_in[0].p[2], gs_in[0].val[1], gs_in[0].val[2]);
	if ((edgeTable[gs_in[0].mc_case] & 4) != 0)
	    vertlist[ 2] = VertexInterp(isoLevel, gs_in[0].p[2], gs_in[0].p[3], gs_in[0].val[2], gs_in[0].val[3]);
	if ((edgeTable[gs_in[0].mc_case] & 8) != 0)
	    vertlist[ 3] = VertexInterp(isoLevel, gs_in[0].p[3], gs_in[0].p[0], gs_in[0].val[3], gs_in[0].val[0]);
	if ((edgeTable[gs_in[0].mc_case] & 16) != 0)
	    vertlist[ 4] = VertexInterp(isoLevel, gs_in[0].p[4], gs_in[0].p[5], gs_in[0].val[4], gs_in[0].val[5]);
	if ((edgeTable[gs_in[0].mc_case] & 32) != 0)
	    vertlist[ 5] = VertexInterp(isoLevel, gs_in[0].p[5], gs_in[0].p[6], gs_in[0].val[5], gs_in[0].val[6]);
	if ((edgeTable[gs_in[0].mc_case] & 64) != 0)
	    vertlist[ 6] = VertexInterp(isoLevel, gs_in[0].p[6], gs_in[0].p[7], gs_in[0].val[6], gs_in[0].val[7]);
	if ((edgeTable[gs_in[0].mc_case] & 128) != 0)
	    vertlist[ 7] = VertexInterp(isoLevel, gs_in[0].p[7], gs_in[0].p[4], gs_in[0].val[7], gs_in[0].val[4]);
	if ((edgeTable[gs_in[0].mc_case] & 256) != 0)
	    vertlist[ 8] = VertexInterp(isoLevel, gs_in[0].p[0], gs_in[0].p[4], gs_in[0].val[0], gs_in[0].val[4]);
	if ((edgeTable[gs_in[0].mc_case] & 512) != 0)
	    vertlist[ 9] = VertexInterp(isoLevel, gs_in[0].p[1], gs_in[0].p[5], gs_in[0].val[1], gs_in[0].val[5]);
	if ((edgeTable[gs_in[0].mc_case] & 1024) != 0)
	    vertlist[10] = VertexInterp(isoLevel, gs_in[0].p[2], gs_in[0].p[6], gs_in[0].val[2], gs_in[0].val[6]);
	if ((edgeTable[gs_in[0].mc_case] & 2048) != 0)
	    vertlist[11] = VertexInterp(isoLevel, gs_in[0].p[3], gs_in[0].p[7], gs_in[0].val[3], gs_in[0].val[7]);

	/* Create the triangle */
	for (int i = 0; triTable[gs_in[0].mc_case].tris[i] != -1; i += 3)
	{
		vec3 pos1 = scale * vertlist[triTable[gs_in[0].mc_case].tris[i  ]];
		vec3 pos2 = scale * vertlist[triTable[gs_in[0].mc_case].tris[i+1]];
		vec3 pos3 = scale * vertlist[triTable[gs_in[0].mc_case].tris[i+2]];

		vec3 triNormal = CalculateNormal(pos1, pos2, pos3);

		gl_Position	= projection * view * model * vec4(pos1, 1.0f);
		position = pos1;
		normal = triNormal;
		EmitVertex();

		gl_Position	= projection * view * model *vec4(pos2, 1.0f);
		position = pos2;
		normal = triNormal;
		EmitVertex();

		gl_Position	= projection * view * model *vec4(pos3, 1.0f);
		position = pos3;
		normal = triNormal;
		EmitVertex();

		EndPrimitive();
	}
}
