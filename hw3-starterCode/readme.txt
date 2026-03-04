Assignment #3: Ray tracing

FULL NAME: Pranav Rathod


MANDATORY FEATURES
------------------

<Under "Status" please indicate whether it has been implemented and is
functioning correctly.  If not, please explain the current status.>

Feature:                                 Status: finish? (yes/no)
-------------------------------------    -------------------------
1) Ray tracing triangles                  Yes

2) Ray tracing sphere                     Yes

3) Triangle Phong Shading                 Yes

4) Sphere Phong Shading                   Yes

5) Shadows rays                           Yes

6) Still images                           Yes
   
7) Extra Credit (up to 20 points)
   - Antialiasing (10 points): Implemented 2x2 grid supersampling. 
     Each pixel shoots 4 rays in a uniform grid pattern and averages 
     the results, producing smoother edges throughout the scene.
     See branch: antialiasing

   - Recursive Reflections (10 points): Implemented recursive ray tracing
     for reflective surfaces. The final color is blended as:
     finalColor = (1 - ks) * localPhongColor + ks * reflectedColor
     Recursion depth is capped at 1 bounce.
     See branch: reflections