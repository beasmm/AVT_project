void freeType_init(const std::string font_name, int width, int height, float& char_width, float& char_height);
void RenderText(VSShaderLib& shader, std::string text, float x, float y, float scale, float cR, float cG, float cB);
float TextWidth(const std::string& text, float scale, float char_width);
float TextHeight(const std::string& text, float scale, float char_height);