# Danh sách sequence diagram và prompt vẽ bằng AI

Mục tiêu: Liệt kê các sequence diagram cho MediaPlayerApp và cung cấp prompt để AI vẽ mỗi sơ đồ ở dạng Mermaid.

Hướng dẫn dùng:
- Sao chép prompt tương ứng và gửi cho AI.
- Yêu cầu AI trả về duy nhất một code block Mermaid bắt đầu bằng `sequenceDiagram`.
- Tên lifelines nên khớp với các lớp/thành phần trong mã nguồn.

## 1) Khởi động ứng dụng
Prompt:
- Bạn là chuyên gia LLD. Hãy vẽ Mermaid sequence diagram mô tả luồng khởi động MediaPlayerApp.
- Tham chiếu mã: [Application.cpp](file:///home/duong/MediaPlayerApp/src/core/Application.cpp), [Application.h](file:///home/duong/MediaPlayerApp/inc/core/Application.h), [ImGuiManager.cpp](file:///home/duong/MediaPlayerApp/src/ui/ImGuiManager.cpp), [UIManager.h](file:///home/duong/MediaPlayerApp/inc/ui/UIManager.h), [AppConfig.cpp](file:///home/duong/MediaPlayerApp/src/config/AppConfig.cpp)
- Lifelines: main, Application, AppConfig, Models, Services, Controllers, UIManager, ImGuiManager
- Yêu cầu: thể hiện tạo Models (QueueModel, PlaybackStateModel, LibraryModel, SystemStateModel), tạo Services (FileScanner, MetadataReader, AudioPlaybackEngine, SerialCommunication), tạo Repositories (Library, Playlist, History), tạo Controllers (Main, Playback, Source, Library, Playlist, Queue, Hardware), khởi tạo UI (ImGuiManager/UIManager), vào event loop.
- Kết quả: Trả về duy nhất một code block Mermaid với sequenceDiagram.

## 2) Điều hướng màn hình
Prompt:
- Hãy vẽ Mermaid sequence diagram cho luồng điều hướng giữa các màn hình qua MainController.navigateTo.
- Tham chiếu mã: [MainController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/MainController.cpp), [MainController.h](file:///home/duong/MediaPlayerApp/inc/controllers/MainController.h)
- Lifelines: UI, MainController, SystemStateModel, Views
- Yêu cầu: UI gửi yêu cầu chuyển màn hình, MainController cập nhật m_currentScreen (MAIN/LIBRARY/PLAYLIST/QUEUE/SCAN), kích hoạt view tương ứng.
- Kết quả: Trả về duy nhất một code block Mermaid với sequenceDiagram.

## 3) Quét thư viện
Prompt:
- Vẽ Mermaid sequence diagram cho luồng quét nguồn media: SourceController gọi FileScanner, đọc metadata, lưu vào LibraryRepository/LibraryModel, cập nhật UI.
- Tham chiếu mã: [SourceController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/SourceController.cpp), [FileScanner.cpp](file:///home/duong/MediaPlayerApp/src/services/FileScanner.cpp), [MetadataReader.cpp](file:///home/duong/MediaPlayerApp/src/services/MetadataReader.cpp), [LibraryRepository.cpp](file:///home/duong/MediaPlayerApp/src/repositories/LibraryRepository.cpp), [LibraryModel.cpp](file:///home/duong/MediaPlayerApp/src/models/LibraryModel.cpp)
- Lifelines: UI, SourceController, FileScanner, MetadataReader, LibraryRepository, LibraryModel, AppConfig
- Yêu cầu: hiển thị callback tiến độ từ FileScanner, đọc metadata cho các file hợp lệ, cập nhật LibraryRepository/Model, UI Library nhận thay đổi. Thêm alt cho file không hỗ trợ theo AppConfig.
- Kết quả: Trả về duy nhất một code block Mermaid với sequenceDiagram.

## 4) Thêm file vào hàng đợi từ Library
Prompt:
- Vẽ Mermaid sequence diagram khi người dùng chọn media từ Library để thêm vào Queue.
- Tham chiếu mã: [LibraryController.h](file:///home/duong/MediaPlayerApp/inc/controllers/LibraryController.h), [QueueController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/QueueController.cpp), [QueueModel.cpp](file:///home/duong/MediaPlayerApp/src/models/QueueModel.cpp)
- Lifelines: LibraryScreen, LibraryController, QueueController, QueueModel, NowPlayingBar
- Yêu cầu: LibraryScreen gửi media chọn, QueueController thêm vào QueueModel, UI NowPlaying cập nhật số lượng và mục hiện tại.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 5) Phát media (happy path)
Prompt:
- Vẽ Mermaid sequence diagram cho luồng phát media chuẩn từ Queue.
- Tham chiếu mã: [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp), [PlaybackStateModel.cpp](file:///home/duong/MediaPlayerApp/src/models/PlaybackStateModel.cpp), [HistoryRepository.cpp](file:///home/duong/MediaPlayerApp/src/repositories/HistoryRepository.cpp)
- Lifelines: UI, QueueModel, PlaybackController, AudioPlaybackEngine, PlaybackStateModel, HistoryRepository, NowPlayingBar
- Yêu cầu: lấy item hiện tại từ QueueModel, gọi play trên PlaybackController/Engine, cập nhật PlaybackStateModel, ghi HistoryRepository, UI đồng bộ trạng thái/timecode.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 6) Tạm dừng/Tiếp tục phát
Prompt:
- Vẽ Mermaid sequence diagram cho pause/resume từ UI hoặc phần cứng.
- Tham chiếu mã: [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp), [SerialCommunication.cpp](file:///home/duong/MediaPlayerApp/src/services/SerialCommunication.cpp), [HardwareController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/HardwareController.cpp)
- Lifelines: UI, SerialCommunication, HardwareController, PlaybackController, AudioPlaybackEngine, PlaybackStateModel, NowPlayingBar
- Yêu cầu: UI hoặc tín hiệu phần cứng kích hoạt hành động, PlaybackController gọi Engine pause/resume, cập nhật PlaybackStateModel, UI hiển thị thay đổi.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 7) Seek/Jump đến vị trí mới
Prompt:
- Vẽ Mermaid sequence diagram cho thao tác seek từ UI slider.
- Tham chiếu mã: [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp)
- Lifelines: UI, PlaybackController, AudioPlaybackEngine, PlaybackStateModel
- Yêu cầu: UI gửi timecode mới, PlaybackController điều phối Engine seek, cập nhật StateModel, UI phản ánh vị trí mới.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 8) Quản lý Playlist
Prompt:
- Vẽ Mermaid sequence diagram cho thêm/xóa/lưu playlist.
- Tham chiếu mã: [PlaylistController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/PlaylistController.cpp), [PlaylistRepository.cpp](file:///home/duong/MediaPlayerApp/src/repositories/PlaylistRepository.cpp), [PlaylistModel.cpp](file:///home/duong/MediaPlayerApp/src/models/PlaylistModel.cpp)
- Lifelines: UI Playlist, PlaylistController, PlaylistRepository, PlaylistModel
- Yêu cầu: UI thực hiện thao tác, Controller cập nhật Repository/Model, UI nhận danh sách mới.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 9) Đọc metadata chi tiết một file
Prompt:
- Vẽ Mermaid sequence diagram cho luồng đọc metadata khi người dùng mở chi tiết media.
- Tham chiếu mã: [MetadataReader.cpp](file:///home/duong/MediaPlayerApp/src/services/MetadataReader.cpp), [IMetadataReader.h](file:///home/duong/MediaPlayerApp/inc/services/IMetadataReader.h), [MetadataModel.cpp](file:///home/duong/MediaPlayerApp/src/models/MetadataModel.cpp), [VideoMetadataReader.cpp](file:///home/duong/MediaPlayerApp/src/services/VideoMetadataReader.cpp)
- Lifelines: UI, MetadataReader, MetadataModel, AppConfig
- Yêu cầu: xác định loại media, đọc metadata phù hợp (audio/video), cập nhật MetadataModel, UI hiển thị kết quả. Thêm alt cho lỗi đọc.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 10) Tương tác phần cứng
Prompt:
- Vẽ Mermaid sequence diagram cho pipeline tín hiệu phần cứng tới hành động điều khiển.
- Tham chiếu mã: [SerialCommunication.cpp](file:///home/duong/MediaPlayerApp/src/services/SerialCommunication.cpp), [HardwareController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/HardwareController.cpp), [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h)
- Lifelines: SerialCommunication, HardwareController, PlaybackController, QueueController, MainController
- Yêu cầu: nhận và parse tín hiệu, ánh xạ sang hành động (play/pause/next/prev/seek/navigate), gọi controller tương ứng.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 11) Thiết lập cấu hình
Prompt:
- Vẽ Mermaid sequence diagram cho thay đổi cấu hình từ UI Settings và áp dụng vào hệ thống.
- Tham chiếu mã: [AppConfig.cpp](file:///home/duong/MediaPlayerApp/src/config/AppConfig.cpp), [FileScanner.cpp](file:///home/duong/MediaPlayerApp/src/services/FileScanner.cpp), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp)
- Lifelines: UI Settings, AppConfig, FileScanner, MetadataReader, AudioPlaybackEngine
- Yêu cầu: UI cập nhật AppConfig (ví dụ danh sách extension), các services đọc cấu hình mới và áp dụng. Thêm alt cho cấu hình không hợp lệ.
- Kết quả: Code block Mermaid với sequenceDiagram.

## 12) Shutdown an toàn
Prompt:
- Vẽ Mermaid sequence diagram cho luồng tắt ứng dụng an toàn.
- Tham chiếu mã: [Application.cpp](file:///home/duong/MediaPlayerApp/src/core/Application.cpp), [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h), [HistoryRepository.cpp](file:///home/duong/MediaPlayerApp/src/repositories/HistoryRepository.cpp)
- Lifelines: UI, Application, Controllers, Services, Repositories
- Yêu cầu: Application.shutdown yêu cầu controllers/services dừng, persist cần thiết (ví dụ lịch sử), giải phóng tài nguyên UI.
- Kết quả: Code block Mermaid với sequenceDiagram.

---

## Tuỳ chọn A) Đồng bộ AV
Prompt:
- Vẽ Mermaid sequence diagram cho đồng bộ audio/video với AVSyncClock khi phát video.
- Tham chiếu mã: [AVSyncClock.cpp](file:///home/duong/MediaPlayerApp/src/utils/AVSyncClock.cpp), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp), [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h)
- Lifelines: PlaybackController, AudioPlaybackEngine, VideoRenderer, AVSyncClock
- Yêu cầu: thể hiện clock cập nhật và điều chỉnh phát để đồng bộ khung hình và âm thanh.
- Kết quả: Code block Mermaid với sequenceDiagram.

## Tuỳ chọn B) Xử lý định dạng không hỗ trợ
Prompt:
- Vẽ Mermaid sequence diagram cho trường hợp phát hiện định dạng không hỗ trợ khi scan/đọc phát.
- Tham chiếu mã: [AppConfig.cpp](file:///home/duong/MediaPlayerApp/src/config/AppConfig.cpp), [FileScanner.cpp](file:///home/duong/MediaPlayerApp/src/services/FileScanner.cpp), [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp)
- Lifelines: UI, SourceController, FileScanner, AppConfig, PlaybackController
- Yêu cầu: kiểm tra extension với AppConfig, nếu không hỗ trợ thì UI thông báo/fallback, nếu hỗ trợ thì tiếp tục bình thường (alt).
- Kết quả: Code block Mermaid với sequenceDiagram.

## Tuỳ chọn C) Kết thúc bài hát và lưu lịch sử
Prompt:
- Vẽ Mermaid sequence diagram cho sự kiện track-end: Engine phát tín hiệu, Controller cập nhật lịch sử và chuyển bài.
- Tham chiếu mã: [AudioPlaybackEngine.cpp](file:///home/duong/MediaPlayerApp/src/services/AudioPlaybackEngine.cpp), [PlaybackController.h](file:///home/duong/MediaPlayerApp/inc/controllers/PlaybackController.h), [HistoryRepository.cpp](file:///home/duong/MediaPlayerApp/src/repositories/HistoryRepository.cpp)
- Lifelines: AudioPlaybackEngine, PlaybackController, HistoryRepository, QueueModel, UI
- Yêu cầu: xử lý track-end, lưu HistoryRepository, lấy track kế tiếp từ QueueModel, UI cập nhật.
- Kết quả: Code block Mermaid với sequenceDiagram.

## Tuỳ chọn D) Điều hướng từ NowPlayingBar/QueuePanel
Prompt:
- Vẽ Mermaid sequence diagram cho thao tác điều hướng/phát từ các panel phụ.
- Tham chiếu mã: [NowPlayingBar.cpp](file:///home/duong/MediaPlayerApp/src/views/NowPlayingBar.cpp), [QueuePanel.cpp](file:///home/duong/MediaPlayerApp/src/views/QueuePanel.cpp), [QueueController.cpp](file:///home/duong/MediaPlayerApp/src/controllers/QueueController.cpp)
- Lifelines: NowPlayingBar, QueuePanel, QueueController, PlaybackController, MainController
- Yêu cầu: người dùng thao tác trên panel, gọi các controller tương ứng, UI cập nhật trạng thái và màn hình nếu cần.
- Kết quả: Code block Mermaid với sequenceDiagram.
