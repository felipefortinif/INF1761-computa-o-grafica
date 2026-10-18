#include "grid.h"
#include "error.h"

Grid::Grid (int nx, int ny) : m_nx(nx), m_ny(ny)
{
  if (nx <= 0 || ny <= 0) Error::Fatal("Grid needs nx > 0 and ny > 0");

  m_coords.resize(2 * VertexCount());
  m_texcoords.resize(2 * VertexCount());
  float dx = 1.0f / nx;
  float dy = 1.0f / ny;
  int nc = 0;
  for (int j = 0; j <= ny; j++) {
    for (int i = 0; i <= nx; i++) {
      m_coords[nc + 0] = i * dx;
      m_coords[nc + 1] = j * dy;
      m_texcoords[nc + 0] = i * dx;
      m_texcoords[nc + 1] = 1.0f - j * dy;   // t cresce para baixo
      nc += 2;
    }
  }

  auto findex = [nx](int i, int j) { return j * (nx + 1) + i; };
  m_indices.resize(IndexCount());
  int ni = 0;
  for (int j = 0; j < ny; j++) {
    for (int i = 0; i < nx; i++) {
      m_indices[ni + 0] = findex(i, j);
      m_indices[ni + 1] = findex(i + 1, j);
      m_indices[ni + 2] = findex(i + 1, j + 1);
      m_indices[ni + 3] = findex(i, j);
      m_indices[ni + 4] = findex(i + 1, j + 1);
      m_indices[ni + 5] = findex(i, j + 1);
      ni += 6;
    }
  }
}

int Grid::GetNx () const { return m_nx; }
int Grid::GetNy () const { return m_ny; }
int Grid::VertexCount () const { return (m_nx + 1) * (m_ny + 1); }
const std::vector<float>& Grid::GetCoords () const { return m_coords; }
const std::vector<float>& Grid::GetTexCoords () const { return m_texcoords; }
int Grid::IndexCount () const { return 6 * m_nx * m_ny; }
const std::vector<uint32_t>& Grid::GetIndices () const { return m_indices; }
