# Wi-Fi Tabanlı Jiroskobik Gonyometre Sistemi

Bu proje, insan vücudundaki tek bir eklemin (örneğin dirsek) açısını, ivmesini ve açısal hızını ölçebilen; jiroskopik sensör ve Wi-Fi haberleşme yeteneğine sahip bir mikrodenetleyicili gonyometre sisteminin tasarım ve prototip üretimini kapsamaktadır. 

Bu çalışma, Danışman Hocam **Doç. Dr. Cabbar Veysel Baysal**'ın rehberliğinde, bitirme tezi kapsamında gerçekleştirilmiştir.

Ayrıca proje süresince değerli katkıları ve desteği için arkadaşım **Muhammed Muqtar Fasasi**’ye özellikle teşekkür ederim.

## 🎯 Proje Hedefi

- Taşınabilir ve düşük maliyetli bir hareket izleme sistemi geliştirmek  
- Rehabilitasyon, ortopedik değerlendirme ve biyomekanik analizlerde kullanılabilir bir cihaz sunmak  
- Gerçek zamanlı ve kablosuz veri iletimi ile kullanıcı dostu bir web arayüzü sağlamak

## 🛠️ Kullanılan Teknolojiler

- **MPU9250** (ivmeölçer + jiroskop + manyetometre)
- **NodeMCU ESP8266** (Wi-Fi destekli mikrodenetleyici)
- **HTML + JavaScript (Chart.js)** ile tarayıcı tabanlı görselleştirme

## 🔍 Teknik Özellikler

- Gerçek zamanlı açı, ivme ve açısal hız ölçümü  
- Wi-Fi üzerinden kablosuz bağlantı  
- Kalibrasyon ve hareketli ortalama filtreleme algoritmaları  
- Tarayıcı üzerinden anlık veri takibi (özel yazılım gerekmez)  

## ✅ Sonuç

Testler sırasında sistem ±0.4° hata payı ile yüksek doğrulukta ölçümler sunmuştur. Kablosuz yapısı, düşük maliyeti ve kolay kullanımı ile fizyoterapi, kişisel hareket takibi ve bilimsel araştırmalar gibi alanlarda kullanılabilir potansiyele sahiptir.
