#!/usr/bin/python3
import os
import sys

# UPLOAD_DIR: lister.py'nin bulunduğu yerden (www/cgi) uploads klasörüne gitmeli
# Unutmayın, bu dizin, POST'un kaydettiği yer olan www/uploads/uploads olmalı.
UPLOAD_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../uploads"))
UPLOADS_ENDPOINT = "/uploads"

def generate_gallery_html():
    """
    Dosya listesini okur ve tüm sayfayı dinamik olarak üretir.
    """
    files = []
    try:
        # Yalnızca dosyaları listele, klasörleri ve gizli dosyaları atla
        files = [f for f in os.listdir(UPLOAD_DIR) if os.path.isfile(os.path.join(UPLOAD_DIR, f))]
    except Exception as e:
        # Eğer klasör yoksa veya okunamıyorsa hata döndür
        return f"<h1>Error: Could not read the uploads directory! ({e})</h1>"

    table_content = ""
    if not files:
        table_content = '<tr><td colspan="3" style="text-align: center;">No files uploaded yet.</td></tr>'
    else:
        for filename in files:
            # Görüntüleme URL'si (GET isteği): /uploads/dosya_adi
            view_url = f"{UPLOADS_ENDPOINT}/{filename}"

            # Tablo satırını oluştur
            table_content += f"""
            <tr>
                <td><a href="{view_url}" target="_blank">{filename}</a></td>
                <td><button class="view-btn" onclick="window.open('{view_url}', '_blank')">View</button></td>
                <td><button class="delete-btn" onclick="deleteFile('{filename}')">Delete</button></td>
            </tr>
            """

    # Tam sayfa çıktısı
    return f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Uploads Gallery & Delete</title>
    <style>
        /* CSS Stilleri */
        body {{ font-family: 'Segoe UI', sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; min-height: 100vh; background-color: #f4f4f9; padding: 20px; }}
        .container {{ background: white; padding: 40px; border-radius: 10px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1); text-align: center; }}
        h1 {{ color: #dc3545; margin-bottom: 30px; }}
        #message {{ margin-bottom: 20px; color: #dc3545; font-weight: bold; }}
        table {{ width: 100%; border-collapse: collapse; margin-top: 20px; }}
        th, td {{ padding: 12px; border: 1px solid #ddd; text-align: left; }}
        th {{ background-color: #f2f2f2; }}
        .delete-btn {{ background-color: #dc3545; color: white; border: none; padding: 8px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }}
        .delete-btn:hover {{ background-color: #c82333; }}
        .view-btn {{ background-color: #008CBA; color: white; border: none; padding: 8px 15px; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }}
        .view-btn:hover {{ background-color: #0056b3; }}
        #backButton {{ position: absolute; top: 20px; left: 20px; background-color: #555; color: white; border: none; padding: 10px 20px; border-radius: 5px; cursor: pointer; text-decoration: none; }}
    </style>
</head>
<body>
    <a href="/">
        <button id="backButton">⬅ Back to Main Page</button>
    </a>
    <div class="container">
        <h1>🗑 Uploads Gallery and Delete Test</h1>
        <p>File list generated dynamically by <code>lister.py</code> CGI script.</p>
        <div id="message"></div>

        <table id="fileListTable">
            <thead>
                <tr>
                    <th>File Name</th>
                    <th>View</th>
                    <th>Action</th>
                </tr>
            </thead>
            <tbody>
                {table_content}
            </tbody>
        </table>
    </div>
<script>
    // DELETE İşlevi (JavaScript)
    const messageDiv = document.getElementById("message");
    const UPLOADS_ENDPOINT = "{UPLOADS_ENDPOINT}";

    async function deleteFile(fileName) {{
        if (!confirm(`Are you sure you want to delete ${{fileName}}?`)) return;

        messageDiv.textContent = `Attempting to delete ${{fileName}}...`;

        try {{
            // Webserv'e DELETE isteği gönder
            const response = await fetch(`${{UPLOADS_ENDPOINT}}/${{fileName}}`, {{
                method: 'DELETE'
            }});

            // KRİTİK DÜZELTME: 204 No Content yanıtını Başarılı kabul et
            // response.ok sadece 200-299 aralığını kapsar, ancak 204'ün içeriği yoktur.
            if (response.ok || response.status === 204) {{
                // response.statusText, 204 için "No Content" veya "Unknown Error" olabilir.
                // Biz sadece kendi başarı mesajımızı gösteriyoruz.
                messageDiv.textContent = `✅ ${{fileName}} deleted successfully. (Status: ${{response.status}})`;

                // Sayfayı yenileyerek listeyi güncelle
                setTimeout(() => {{ window.location.reload(); }}, 1500);
            }} else {{
                // Hata varsa
                messageDiv.textContent = `❌ Error deleting ${{fileName}}: ${{response.status}} ${{response.statusText}}`;
            }}
        }} catch (error) {{
            messageDiv.textContent = `⚠️ DELETE failed: ${{error}}`;
        }}
    }}
</script>
</body>
</html>
"""

# --- Ana Çalıştırma Bloğu ---

# 1. CGI Başlıklarını Yazdır
sys.stdout.write("Content-type: text/html\r\n\r\n")

# 2. Dinamik HTML çıktısını yazdır
sys.stdout.write(generate_gallery_html())
