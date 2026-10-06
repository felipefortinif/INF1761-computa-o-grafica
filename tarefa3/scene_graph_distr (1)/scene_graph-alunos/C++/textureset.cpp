#include "textureset.h"
#include "state.h"
#include "shader.h"

TextureSet::TextureSet (std::initializer_list<TextureItem*> items) : m_items(items.begin(), items.end()) {}

TextureSetPtr TextureSet::Make (std::initializer_list<TextureItem*> items)
{
  return TextureSetPtr(new TextureSet(items));
}

const std::vector<TextureItem*>& TextureSet::GetItems () const { return m_items; }

void TextureSet::Load (StatePtr st) { st->GetShader()->BindTextureSet(st, this); }
void TextureSet::Unload (StatePtr st) { st->GetShader()->UnbindTextureSet(st); }
