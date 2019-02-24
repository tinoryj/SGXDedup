boost 序列化时会跟踪对象，比如 Chunk 中 char _Recipe[8]，原意是此处只保存指针，但在序列化时，serialization 模块会跟踪指针，将 Recipe 一同序列化，导致最终结果过大。所以此处使用 unsigned char 保存指针内容，防止对象跟踪 
