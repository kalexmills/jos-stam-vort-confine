Population dynamics using real-time fluid dynamics.

Note: Failed experiment. Did not make the kind of progress which was expected.

Added / Planned methods:

void live_step ( int N, float * x, float * x0, float * via, float dt )
  Uses the viability matrix, which specifies the viability of population x at every point
  in the matrix, to grow or kill the population in x over the course of time dt.

void fear_step ( int N, float * x, float * u, float * u0, float * v, float * v0, 
                 float * y, float fear, float dt )
  Uses the given fear value to add velocities to u and v so as to push the density stored in
  x away from that stored in y by adding a computed fear force, which is advected through the
  population as normal. The velocities added are proportional to the value of fear and the 
  relative difference in density between x and y. A negative fear value is a "love" value
  which pulls one density towards another. Pulling a density towards itself results in cohesion.
