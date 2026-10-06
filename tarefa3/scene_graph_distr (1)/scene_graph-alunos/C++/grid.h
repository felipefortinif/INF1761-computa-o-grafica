#ifndef GRID_H
#define GRID_H

#include <vector>
#include <cstdint>

// Generic indexed nx-by-ny grid mesh generator, shared by Quad and
// Sphere. nx/ny are cell counts, not vertex counts - there are
// (nx+1)x(ny+1) vertices. Carries two parallel arrays: coords, with y
// growing up (the geometry), and texcoords, with t growing down (the
// WebGPU convention, origin at the image's top-left corner).
class Grid {
  int m_nx, m_ny;
  std::vector<float> m_coords;
  std::vector<float> m_texcoords;
  std::vector<uint32_t> m_indices;
public:
  Grid (int nx, int ny);
  int GetNx () const;
  int GetNy () const;
  int VertexCount () const;
  const std::vector<float>& GetCoords () const;
  // Same order as GetCoords(), but with t mirrored: t = 1 - y, so t = 0
  // is the top of the image.
  const std::vector<float>& GetTexCoords () const;
  int IndexCount () const;
  const std::vector<uint32_t>& GetIndices () const;
};

#endif
