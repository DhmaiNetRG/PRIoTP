results = model.train(
    data=str(yaml_path),
    epochs=50,              # ← More epochs for 22 classes
    patience=5,             # ← Longer patience for complex task
    imgsz=640,               # Or 800 if GPU memory allows
    batch=32,                # ← Reduced batch for better gradient updates
    project="/kaggle/working",
    name="yolo11_blindHelper_22class",
    device=0,
    workers=2,
    exist_ok=True,
    
    # Learning rate - important for multi-class
    lr0=0.01,
    lrf=0.001,               # ← Lower final LR for fine-tuning

    
    # Stronger augmentation for 22 classes
    hsv_h=0.015,             # HSV-Hue augmentation
    hsv_s=0.7,               # HSV-Saturation
    hsv_v=0.4,               # HSV-Value
    degrees=20.0,            # Rotation
    translate=0.2,           # Translation
    scale=0.7,               # Scale
    shear=2.0,               # Shear
    perspective=0.0001,      # Perspective
    flipud=0.0,              # No vertical flip
    fliplr=0.5,              # Horizontal flip
    mosaic=1.0,              # Mosaic (great for multi-class)
    mixup=0.2,               # Mixup
    copy_paste=0.5,          # ← Higher for rare classes
    
    # Multi-class optimization
    optimizer='AdamW',
    weight_decay=0.0005,
    momentum=0.937,
    close_mosaic=10,         # Disable mosaic in last 10 epochs
    
    # Class-specific settings
    cls=0.5,                 # ← Class loss weight (important!)
    box=7.5,                 # Box loss weight
    dfl=1.5,                 # DFL loss weight
    
    # NMS settings for multiple classes
    iou=0.7,                 # IoU threshold
    conf=0.001,              # Low conf during training
    
    # Validation
    val=True,
    save=True,
    save_period=25,
    plots=True,
    
    # Performance
    amp=True,
    rect=False,              # Rectangular training (can help)
    
    # Verbose
    verbose=True,
)