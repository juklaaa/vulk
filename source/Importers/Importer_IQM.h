#pragma once

#include "Common.h"

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