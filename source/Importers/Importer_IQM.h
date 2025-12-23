#pragma once

#include "Common.h"
#include <bit>

struct iqmheader
{
    char magic[16]; // the string "INTERQUAKEMODEL\0", 0 terminated
    uint version; // must be version 2
    uint filesize;
    uint flags;
    uint num_text, ofs_text;
    uint num_meshes, ofs_meshes;
    uint num_vertexarrays, num_vertexes, ofs_vertexarrays;
    uint num_triangles, ofs_triangles, ofs_adjacency;
    uint num_joints, ofs_joints;
    uint num_poses, ofs_poses;
    uint num_anims, ofs_anims;
    uint num_frames, num_framechannels, ofs_frames, ofs_bounds;
    uint num_comment, ofs_comment;
    uint num_extensions, ofs_extensions; // these are stored as a linked list, not as a contiguous array
};

struct iqmjoint
{
    uint name;
    int parent; // parent < 0 means this is a root bone
    float translate[3], rotate[4], scale[3];
    // translate is translation <Tx, Ty, Tz>, and rotate is quaternion rotation <Qx, Qy, Qz, Qw>
    // rotation is in relative/parent local space
    // scale is pre-scaling <Sx, Sy, Sz>
    // output = (input*scale)*rotation + translation
};

struct iqmpose
{
    int parent; // parent < 0 means this is a root bone
    uint channelmask; // mask of which 10 channels are present for this joint pose
    float channeloffset[10], channelscale[10];
    // channels 0..2 are translation <Tx, Ty, Tz> and channels 3..6 are quaternion rotation <Qx, Qy, Qz, Qw>
    // rotation is in relative/parent local space
    // channels 7..9 are scale <Sx, Sy, Sz>
    // output = (input*scale)*rotation + translation
};

struct iqmanim
{
    uint name;
    uint first_frame, num_frames;
    float framerate;
    uint flags;
};

struct iqmmesh
{
    uint name;
    uint material;
    uint first_vertex, num_vertexes;
    uint first_triangle, num_triangles;
};

struct iqmtriangle
{
    uint vertex[3];
};

struct iqmadjacency
{
    uint triangle[3];
};

enum class IQM_VertexArrayType : uint
{
    IQM_POSITION = 0,  // float, 3
    IQM_TEXCOORD = 1,  // float, 2
    IQM_NORMAL = 2,  // float, 3
    IQM_TANGENT = 3,  // float, 4
    IQM_BLENDINDEXES = 4,  // ubyte, 4
    IQM_BLENDWEIGHTS = 5,  // ubyte, 4
    IQM_COLOR = 6,  // ubyte, 4

    // all values up to IQM_CUSTOM are reserved for future use
    // any value >= IQM_CUSTOM is interpreted as CUSTOM type
    // the value then defines an offset into the string table, where offset = value - IQM_CUSTOM
    // this must be a valid string naming the type
    IQM_CUSTOM = 0x10
};

enum class IQM_Format : uint
{
    IQM_BYTE = 0,
    IQM_UBYTE = 1,
    IQM_SHORT = 2,
    IQM_USHORT = 3,
    IQM_INT = 4,
    IQM_UINT = 5,
    IQM_HALF = 6,
    IQM_FLOAT = 7,
    IQM_DOUBLE = 8,
};

struct iqmvertexarray
{
    IQM_VertexArrayType type;   // type or custom name
    uint flags;
    IQM_Format format; // component format
    uint size;   // number of components
    uint offset; // offset to array of tightly packed components, with num_vertexes * size total entries
    // offset must be aligned to max(sizeof(format), 4)
};
