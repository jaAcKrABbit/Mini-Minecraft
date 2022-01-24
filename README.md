# final-project-mini-minecraft-good-game
final-project-mini-minecraft-good-game created by GitHub Classroom

Team Good Game
Partition of Work:

Milestone 1:

Procedural Terrain: Hao Qin

Efficient Terrain Rendering and Chunking: Shineng Tang

Game Engine Tick Function and Player Physics: Wentao Xu

Milestone 2:

MultiThreading: Shineng Tang

Texture: Wentao Xu

Cave System: Hao Qin

Procedural Terrain Implementation:
I used FBM noise for grass because it gives a nice slope for grassland. And I used Worley noise for mountain because it's simple to implement with nice output. For the third noise function I used Smoothstep(Worley) as the t value in LERP.

Efficient Terrain Rendering and Chunking:
For chunking, I inherited chunk class from Drawble. I created vbo data in chunk and set up the vbos ready to be drawn, only to draw quadrangles whose neighbors' blocktype are EMPTY. In this process, edge cases when getting neighboring chunks needed to be taken extra care of. In shadreprogram and drawble class, I set up every thing needed for opengl. In terrain class, I modified draw function to loop through all the chunks in the terrain and use shaderprogam to draw them one by one. For expanding terrains, I checked player position in tick() function to get current chunk position which the player is standing on. And used this current chunk position to check if the surrounding 8 chunks existed or not. If not, instantiate a new chunk and create VBO data for it, so that draw() function can draw it out.

Game Engine Tick Function and Player Physics:
For character's general movement, I used inputBundle to store if the key has been pressed multiple times and then to add the acceleration to the speed. There are two modes in our miniminecraft, the one is the flight mode, which means we donnot need to add gravity to our acceleration. When changed to non-flight mode, I will give a gravity(g) to the acceleartion when the player is in the sky, and when the player reaches the gound, the acceleration of g will be removed.

For the collision detection, I used the method from the slides and use the same method to implement the remove block and place block. I chose to cast rays from the corners of my player's bounding box for collision detection, the codes are offered but I didn't make it because I've run out of time, hopefeully I can make it during the weekend.

MultiThreading:
For terrain expansion, check the surrounding 5x5 terrain zones each frame to see if they have set up blocks. If not, spawn a thread to set blocks up. If already set up blocks, spawn a thread to create vbo data for them. And on the main thread, send vbo data to gpu to draw all the blocks. Also, destroy all the blocks' vbo data if the are too far away from the player for efficiency.

Texture:
For texture, I refered to the homework 4, I split the data_interleave into 2 parts, the first part is opaque data, the other is transparent part, for different part, I will set the uv vector's z and w into different values. For transparent part, I set the w of uv vector to be 0, which means in glsl part, openGL will set it to be transparent based on the w(here I have added alpha statement in mygl::initialize()). For block water and lava, I set the uv's z to be 0, which means they will be animated block, and in glsl, I passed a variabe m_time, which is used to set the animation effect. Also I modified the createTestScene to create different scene.

Cave system:
I used perlinNoise3D from the slides to implement the underground cave. I changed the values of the input so that I can get more appealling cave system in the graph. The video I shot is in the Terrrain branch. I didn't have time working on the postprocessing though.

