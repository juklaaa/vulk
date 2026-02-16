glslc.exe shader.vert -o nmap_v.spv
glslc.exe shader.frag -o nmap_f.spv

glslc.exe offscreenshader.vert -o offscreen_v.spv
glslc.exe offscreenshader.frag -o offscreen_f.spv

glslc.exe shaderSkel.vert -o anim_v.spv

glslc.exe offscreenSkel.vert -o animOffscreen_v.spv

pause