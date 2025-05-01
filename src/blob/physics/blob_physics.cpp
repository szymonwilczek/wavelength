#include "blob_physics.h"
#include <qfuture.h>
#include <qtconcurrentrun.h>

BlobPhysics::BlobPhysics() {
    physics_timer_.start();
    thread_pool_.setMaxThreadCount(QThread::idealThreadCount());
}

void BlobPhysics::UpdatePhysicsOptimized(std::vector<QPointF> &control_points,
                                         const std::vector<QPointF> &target_points,
                                         std::vector<QPointF> &velocity,
                                         const QPointF &blob_center,
                                         const BlobConfig::BlobParameters &params,
                                         const BlobConfig::PhysicsParameters &physics_params) {

    // --- DODANE ZABEZPIECZENIE ---
    // Sprawdź, czy wszystkie wektory mają oczekiwany i spójny rozmiar
    if (const auto expected_size = static_cast<size_t>(params.numPoints); control_points.size() != expected_size ||
                                                                           target_points.size() != expected_size ||
                                                                           velocity.size() != expected_size) {
        qCritical() << "BlobPhysics::updatePhysicsOptimized - Niespójne rozmiary wektorów!";
        qCritical() << "controlPoints:" << control_points.size() << "targetPoints:" << target_points.size() << "velocity:" << velocity.size() << "expected:" << expected_size;
        // assert(false && "Niespójne rozmiary wektorów w updatePhysicsOptimized!"); // Można zostawić lub usunąć asercję tutaj
        return; // <--- DODAJ TĘ LINIĘ
        }
    // --- KONIEC ZABEZPIECZENIA ---

    const size_t num_of_points = control_points.size();

    // Używamy struktur danych zoptymalizowanych pod kątem wydajności pamięci podręcznej
    static std::vector<float> pos_x, pos_y, target_x, target_y, velocity_x, vel_y, force_x, force_y;

    // Rezerwuj pamięć raz
    if (pos_x.size() != num_of_points) {
        pos_x.resize(num_of_points);
        pos_y.resize(num_of_points);
        target_x.resize(num_of_points);
        target_y.resize(num_of_points);
        velocity_x.resize(num_of_points);
        vel_y.resize(num_of_points);
        force_x.resize(num_of_points);
        force_y.resize(num_of_points);
    }

    // Konwertuj dane na zoptymalizowane struktury (SoA - Structure of Arrays)
    const auto center_x = static_cast<float>(blob_center.x());
    const auto center_y = static_cast<float>(blob_center.y());
    const auto radius_threshold = static_cast<float>(params.blobRadius * 1.1f);
    const float radius_threshold_squared = radius_threshold * radius_threshold;
    const auto damping_factor = static_cast<float>(physics_params.damping);
    const auto viscosity = static_cast<float>(physics_params.viscosity);
    const auto velocity_threshold = static_cast<float>(physics_params.velocityThreshold);
    const float velocity_threshold_squared = velocity_threshold * velocity_threshold;
    const auto max_speed = static_cast<float>(params.blobRadius * physics_params.maxSpeed);
    const float max_speed_squared = max_speed * max_speed;

    // Kopiuj dane do zoptymalizowanych buforów
    #pragma omp parallel for
    for (int i = 0; i < num_of_points; ++i) {
        pos_x[i] = static_cast<float>(control_points[i].x());
        pos_y[i] = static_cast<float>(control_points[i].y());
        target_x[i] = static_cast<float>(target_points[i].x());
        target_y[i] = static_cast<float>(target_points[i].y());
        velocity_x[i] = static_cast<float>(velocity[i].x());
        vel_y[i] = static_cast<float>(velocity[i].y());
        force_x[i] = 0.0f;
        force_y[i] = 0.0f;
    }

    bool is_in_motion = false;

    // Przetwarzaj w blokach dla lepszej wydajności cache
    constexpr int block_size = 64;  // Zoptymalizowane pod kątem linii cache

    #pragma omp parallel for reduction(|:isInMotion)
    for (int block_start = 0; block_start < num_of_points; block_start += block_size) {
        const int block_end = std::min(block_start + block_size, static_cast<int>(num_of_points));

        for (int i = block_start; i < block_end; ++i) {
            // Oblicz siłę
            force_x[i] = (target_x[i] - pos_x[i]) * viscosity;
            force_y[i] = (target_y[i] - pos_y[i]) * viscosity;

            // Oblicz wektor do środka
            const float vector_to_center_x = center_x - pos_x[i];
            const float vector_to_center_y = center_y - pos_y[i];

            if (const float distance_squared = vector_to_center_x * vector_to_center_x + vector_to_center_y * vector_to_center_y; distance_squared > radius_threshold_squared) {
                // Fast inverse square root approximation
                const float inversed_distance = 1.0f / sqrtf(distance_squared);
                const float factor = 0.03f * inversed_distance;
                force_x[i] += vector_to_center_x * factor;
                force_y[i] += vector_to_center_y * factor;
            }

            // Aktualizuj prędkość
            velocity_x[i] += force_x[i];
            vel_y[i] += force_y[i];

            // Zastosuj tłumienie
            velocity_x[i] *= damping_factor;
            vel_y[i] *= damping_factor;

            // Sprawdź prędkość

            if (const float speed_squared = velocity_x[i] * velocity_x[i] + vel_y[i] * vel_y[i]; speed_squared < velocity_threshold_squared) {
                velocity_x[i] = 0.0f;
                vel_y[i] = 0.0f;
            } else {
                is_in_motion = true;

                if (speed_squared > max_speed_squared) {
                    const float scaleFactor = max_speed / sqrtf(speed_squared);
                    velocity_x[i] *= scaleFactor;
                    vel_y[i] *= scaleFactor;
                }
            }

            // Aktualizuj pozycję
            pos_x[i] += velocity_x[i];
            pos_y[i] += vel_y[i];
        }
    }

    // Kopiuj dane z powrotem
    #pragma omp parallel for
    for (int i = 0; i < num_of_points; ++i) {
        control_points[i].rx() = pos_x[i];
        control_points[i].ry() = pos_y[i];
        velocity[i].rx() = velocity_x[i];
        velocity[i].ry() = vel_y[i];
    }

    if (!is_in_motion) {
        StabilizeBlob(control_points, blob_center, params.blobRadius, physics_params.stabilizationRate);
    }

    ValidateAndRepairControlPoints(control_points, velocity, blob_center, params.blobRadius);
}

void BlobPhysics::UpdatePhysicsParallel(std::vector<QPointF>& control_points,
                                      std::vector<QPointF>& target_points,
                                      std::vector<QPointF>& velocity,
                                      const QPointF& blob_center,
                                      const BlobConfig::BlobParameters& params,
                                      const BlobConfig::PhysicsParameters& physics_params) {

    // --- DODANE ZABEZPIECZENIE ---
    const size_t expected_size = static_cast<size_t>(params.numPoints);
    if (control_points.size() != expected_size ||
        target_points.size() != expected_size ||
        velocity.size() != expected_size) {
        qCritical() << "BlobPhysics::updatePhysicsParallel - Niespójne rozmiary wektorów PRZED odczytem!";
        qCritical() << "controlPoints:" << control_points.size() << "targetPoints:" << target_points.size() << "velocity:" << velocity.size() << "expected:" << expected_size;
        return; // Zatrzymujemy, jeśli rozmiary są niespójne na starcie
        }
    // Odczytaj rozmiary *przed* uruchomieniem wątków
    const size_t initial_control_points_size = control_points.size();
    const size_t initial_target_points_size = target_points.size();

    // Sprawdzenie spójności (można usunąć, bo jest już wyżej)
    if (const size_t initial_velocity_size = velocity.size(); initial_control_points_size != expected_size ||
                                                            initial_target_points_size != expected_size ||
                                                            initial_velocity_size != expected_size) {
        qCritical() << "BlobPhysics::updatePhysicsParallel - Niespójne rozmiary wektorów! (Asercja)";
        qCritical() << "controlPoints:" << initial_control_points_size << "targetPoints:" << initial_target_points_size << "velocity:" << initial_velocity_size << "expected:" << expected_size;
        // assert(false && "Niespójne rozmiary wektorów w updatePhysicsParallel!"); // Usunięto asercję
        return;
        }
    // --- KONIEC ZABEZPIECZENIA ---

    // Użyj rozmiaru odczytanego pod blokadą
    const size_t num_of_points = initial_control_points_size;

    if (num_of_points < 24) { // Lub inna wartość progowa używana później
        // Jeśli jest zbyt małe, od razu przejdź do wersji sekwencyjnej
        UpdatePhysics(control_points, target_points, velocity, blob_center, params, physics_params);
        return;
    }

    // Użyj SIMD-friendly struktury danych gdy liczba punktów jest duża
    if (num_of_points > 64) {
        UpdatePhysicsOptimized(control_points, target_points, velocity, blob_center, params, physics_params);
        return;
    }

    const int num_of_threads = qMin(thread_pool_.maxThreadCount(), static_cast<int>(num_of_points / 8 + 1));
    const int batch_size = std::max(1, static_cast<int>(num_of_points) / thread_pool_.maxThreadCount());
    std::vector<std::pair<int, int>> ranges;
    // Upewnij się, że pętla używa int, jeśli numPoints może być duże, rozważ size_t
    for (int i = 0; i < static_cast<int>(num_of_points); i += batch_size) {
        ranges.push_back({i, std::min(i + batch_size, static_cast<int>(num_of_points))});
    }


    // Prekalkujemy stałe wartości dla wszystkich wątków
    const double radius_threshold = params.blobRadius * 1.1;
    const double radius_threshold_squared = radius_threshold * radius_threshold;
    const double damping_factor = physics_params.damping;
    const double max_speed = params.blobRadius * physics_params.maxSpeed;
    const double velocity_threshold_squared = physics_params.velocityThreshold * physics_params.velocityThreshold;
    const double max_speed_squared = max_speed * max_speed;

    // Użyj statycznego bufora dla poprzednich prędkości, aby uniknąć realokacji
    if (velocity.size() != num_of_points) {
        qCritical() << "BlobPhysics::updatePhysicsParallel - Rozmiar 'velocity' (" << velocity.size() << ") zmienił się przed kopiowaniem! Oczekiwano:" << num_of_points;
        assert(false && "Rozmiar 'velocity' zmienił się przed kopiowaniem!");
        return;
    }
    // --- Koniec sprawdzenia ---

    static std::vector<QPointF> previous_velocity;
    if (previous_velocity.size() != num_of_points) {
        previous_velocity.resize(num_of_points);
    }
    // --- Dodatkowe sprawdzenie po resize i przed copy ---
    if (velocity.size() != previous_velocity.size()) {
        qCritical() << "BlobPhysics::updatePhysicsParallel - Niespójne rozmiary 'velocity' (" << velocity.size()
                    << ") i 'previousVelocity' (" << previous_velocity.size() << ") przed std::copy!";
        assert(false && "Niespójne rozmiary velocity i previousVelocity przed std::copy!");
        return;
    }
    // --- Koniec sprawdzenia ---
    std::ranges::copy(velocity, previous_velocity.begin());


    std::atomic is_in_motion(false);
    QVector<QFuture<void>> futures;
    futures.reserve(num_of_threads);

    for (int t = 0; t < num_of_threads; ++t) {
        constexpr double prev_velocity_blend = 0.2;
        constexpr double velocity_blend = 0.8;
        const size_t start_idx = t * (num_of_points / num_of_threads);
        const size_t end_idx = (t == num_of_threads-1) ? num_of_points : (t+1) * (num_of_points / num_of_threads);

        // --- Sprawdzenie poprawności zakresu wątku ---
        if (start_idx >= num_of_points || end_idx > num_of_points || start_idx > end_idx) {
            qCritical() << "BlobPhysics::updatePhysicsParallel - Nieprawidłowy zakres dla wątku" << t << "! startIdx:" << start_idx << "endIdx:" << end_idx << "numPoints:" << num_of_points;
            assert(false && "Nieprawidłowy zakres dla wątku!");
            continue; // Pomiń ten wątek
        }
        // --- Koniec sprawdzenia ---

        futures.append(QtConcurrent::run(&thread_pool_, [&, start_idx, end_idx, radius_threshold_squared,
                                        damping_factor, velocity_blend, prev_velocity_blend,
                                        velocity_threshold_squared, max_speed_squared]() {
            // Prekalkujemy środek jeden raz dla każdego wątku
            const double center_x = blob_center.x();
            const double center_y = blob_center.y();

            for (size_t batch_start = start_idx; batch_start < end_idx; batch_start += batch_size) {
                const size_t batch_end = qMin(batch_start + batch_size, end_idx);

                if (batch_start >= num_of_points || batch_end > num_of_points || batch_start > batch_end) {
                    qCritical() << "BlobPhysics::updatePhysicsParallel - Nieprawidłowy zakres partii! batchStart:" << batch_start << "batchEnd:" << batch_end << "numPoints:" << num_of_points << "Wątek:" << QThread::currentThreadId();
                    assert(false && "Nieprawidłowy zakres partii!");
                    continue; // Pomiń tę partię
                 }

                for (size_t i = batch_start; i < batch_end; ++i) {

                    const size_t current_control_points_size = control_points.size(); // Odczytaj AKTUALNY rozmiar
                    if (i >= current_control_points_size) {
                        qCritical() << "BlobPhysics::updatePhysicsParallel - WYKRYTO DOSTĘP POZA ZAKRES controlPoints! Index 'i':" << i
                                    << "Aktualny rozmiar:" << current_control_points_size
                                    << "Oczekiwany rozmiar (numPoints):" << num_of_points
                                    << "Zakres wątku:" << start_idx << "-" << end_idx
                                    << "Zakres partii:" << batch_start << "-" << batch_end
                                    << "Wątek ID:" << QThread::currentThreadId();
                         assert(false && "Dostęp poza zakres controlPoints w pętli równoległej!");
                        return; // Zakończ to zadanie, aby uniknąć crasha
                    }

                    QPointF& current_point = control_points[i];

                    const size_t current_target_points_size = target_points.size();
                    if (i >= current_target_points_size) {
                         qCritical() << "BlobPhysics::updatePhysicsParallel - Dostęp poza zakres targetPoints! Index 'i':" << i << "Aktualny rozmiar:" << current_target_points_size << "Wątek ID:" << QThread::currentThreadId();
                         assert(false && "Dostęp poza zakres targetPoints!");
                         return;
                    }
                    const size_t current_velocity_size = velocity.size();
                     if (i >= current_velocity_size) {
                         qCritical() << "BlobPhysics::updatePhysicsParallel - Dostęp poza zakres velocity! Index 'i':" << i << "Aktualny rozmiar:" << current_velocity_size << "Wątek ID:" << QThread::currentThreadId();
                         assert(false && "Dostęp poza zakres velocity!");
                         return;
                    }
                    const size_t previous_velocity_size = previous_velocity.size();
                     if (i >= previous_velocity_size) {
                         qCritical() << "BlobPhysics::updatePhysicsParallel - Dostęp poza zakres previousVelocity! Index 'i':" << i << "Aktualny rozmiar:" << previous_velocity_size << "Wątek ID:" << QThread::currentThreadId();
                         assert(false && "Dostęp poza zakres previousVelocity!");
                         return;
                    }

                    QPointF& current_target = target_points[i];
                    QPointF& current_velocity = velocity[i];
                    const QPointF& prev_velocity = previous_velocity[i];

                    // Oblicz siłę bezpośrednio bez tworzenia pośrednich obiektów
                    double force_x = (current_target.x() - current_point.x()) * physics_params.viscosity;
                    double force_y = (current_target.y() - current_point.y()) * physics_params.viscosity;

                    // Oblicz wektor do środka bez tworzenia obiektu QPointF
                    const double vector_to_center_x = center_x - current_point.x();
                    const double vector_to_center_y = center_y - current_point.y();
                    const double dist_from_center_squared = vector_to_center_x*vector_to_center_x + vector_to_center_y*vector_to_center_y;

                    if (dist_from_center_squared > radius_threshold_squared) {
                        // Fast inverse square root approximation można zastosować zamiast qSqrt dla wyższej wydajności
                        const double factor = 0.03 / qSqrt(dist_from_center_squared);
                        force_x += vector_to_center_x * factor;
                        force_y += vector_to_center_y * factor;
                    }

                    current_velocity.rx() += force_x;
                    current_velocity.ry() += force_y;

                    // Blend prędkości
                    current_velocity.rx() = current_velocity.x() * velocity_blend + prev_velocity.x() * prev_velocity_blend;
                    current_velocity.ry() = current_velocity.y() * velocity_blend + prev_velocity.y() * prev_velocity_blend;

                    // Zastosuj tłumienie
                    current_velocity *= damping_factor;

                    const double speed_squared = current_velocity.x()*current_velocity.x() +
                                         current_velocity.y()*current_velocity.y();

                    if (speed_squared < velocity_threshold_squared) {
                        current_velocity = QPointF(0, 0);
                    } else {
                        is_in_motion = true;

                        if (speed_squared > max_speed_squared) {
                            const double scale_factor = max_speed / qSqrt(speed_squared);
                            current_velocity *= scale_factor;
                        }
                    }

                    // Aktualizuj pozycję punktu
                    current_point.rx() += current_velocity.x();
                    current_point.ry() += current_velocity.y();
                }
            }
        }));
    }

    for (auto& future : futures) {
        future.waitForFinished();
    }

    if (!is_in_motion) {
        StabilizeBlob(control_points, blob_center, params.blobRadius, physics_params.stabilizationRate);
    }

    ValidateAndRepairControlPoints(control_points, velocity, blob_center, params.blobRadius);
}

void BlobPhysics::UpdatePhysics(std::vector<QPointF> &control_points,
                                const std::vector<QPointF> &target_points,
                                std::vector<QPointF> &velocity,
                                const QPointF &blob_center,
                                const BlobConfig::BlobParameters &params,
                                const BlobConfig::PhysicsParameters &physics_params) {

    // --- DODANE ZABEZPIECZENIE ---
    if (const size_t expected_size = static_cast<size_t>(params.numPoints); control_points.size() != expected_size ||
                                                                           target_points.size() != expected_size ||
                                                                           velocity.size() != expected_size) {
        qCritical() << "BlobPhysics::updatePhysics - Niespójne rozmiary wektorów!";
        assert(false && "Niespójne rozmiary wektorów w updatePhysics!");
        return;
        }
    // --- KONIEC ZABEZPIECZENIA ---

    static std::vector<QPointF> previous_velocity;
    if (previous_velocity.size() != velocity.size()) {
        previous_velocity.resize(velocity.size());
    }

    const double radius_threshold = params.blobRadius * 1.1;
    const double damping_factor = physics_params.damping;
    const double max_speed = params.blobRadius * physics_params.maxSpeed;

    bool is_in_motion = false;

    for (size_t i = 0; i < control_points.size(); ++i) {
        constexpr double prev_velocity_blend = 0.2;
        constexpr double velocity_blend = 0.8;
        previous_velocity[i] = velocity[i];

        QPointF force = (target_points[i] - control_points[i]) * physics_params.viscosity;

        QPointF vector_to_center = blob_center - control_points[i];

        if (const double dist_from_center_squared = vector_to_center.x()*vector_to_center.x() + vector_to_center.y()*vector_to_center.y(); dist_from_center_squared > radius_threshold * radius_threshold) {
            const double factor = 0.03 / qSqrt(dist_from_center_squared);
            force.rx() += vector_to_center.x() * factor;
            force.ry() += vector_to_center.y() * factor;
        }

        velocity[i] += force;

        // Miksowanie prędkości
        velocity[i].rx() = velocity[i].x() * velocity_blend + previous_velocity[i].x() * prev_velocity_blend;
        velocity[i].ry() = velocity[i].y() * velocity_blend + previous_velocity[i].y() * prev_velocity_blend;

        // Tłumienie
        velocity[i] *= damping_factor;

        const double speed_squared = velocity[i].x()*velocity[i].x() + velocity[i].y()*velocity[i].y();

        if (const double threshold_squared = physics_params.velocityThreshold * physics_params.velocityThreshold; speed_squared < threshold_squared) {
            velocity[i] = QPointF(0, 0);
        } else {
            is_in_motion = true;

            if (speed_squared > max_speed * max_speed) {
                const double scaleFactor = max_speed / qSqrt(speed_squared);
                velocity[i] *= scaleFactor;
            }
        }

        control_points[i] += velocity[i];
    }

    if (!is_in_motion) {
        StabilizeBlob(control_points, blob_center, params.blobRadius, physics_params.stabilizationRate);
    }

    if (Q_UNLIKELY(ValidateAndRepairControlPoints(control_points, velocity, blob_center, params.blobRadius))) {
        qDebug() << "Wykryto nieprawidłowe punkty kontrolne. Zresetowano kształt blobu.";
    }
}

void BlobPhysics::InitializeBlob(std::vector<QPointF>& control_points,
                               std::vector<QPointF>& target_points,
                               std::vector<QPointF>& velocity,
                               QPointF& blob_center,
                               const BlobConfig::BlobParameters& params,
                               const int width, const int height) {

    blob_center = QPointF(width / 2.0, height / 2.0);

    control_points = BlobMath::generateCircularPoints(blob_center, params.blobRadius, params.numPoints);
    target_points = control_points;

    velocity.resize(params.numPoints);
    for (auto& vel : velocity) {
        vel = QPointF(0, 0);
    }
}

void BlobPhysics::HandleBorderCollisions(std::vector<QPointF>& control_points,
                                       std::vector<QPointF>& velocity,
                                       QPointF& blob_center,
                                       const int width, const int height,
                                       const double restitution,
                                       const int padding) {

    const int min_x = padding;
    const int min_y = padding;
    const int max_x = width - padding;
    const int max_y = height - padding;

    for (size_t i = 0; i < control_points.size(); ++i) {
        // Kolizje w osi X
        if (control_points[i].x() < min_x) {
            control_points[i].setX(min_x);
            velocity[i].setX(-velocity[i].x() * restitution);
        }
        else if (control_points[i].x() > max_x) {
            control_points[i].setX(max_x);
            velocity[i].setX(-velocity[i].x() * restitution);
        }

        // Kolizje w osi Y
        if (control_points[i].y() < min_y) {
            control_points[i].setY(min_y);
            velocity[i].setY(-velocity[i].y() * restitution);
        }
        else if (control_points[i].y() > max_y) {
            control_points[i].setY(max_y);
            velocity[i].setY(-velocity[i].y() * restitution);
        }
    }

    if (blob_center.x() < min_x) blob_center.setX(min_x);
    if (blob_center.x() > max_x) blob_center.setX(max_x);
    if (blob_center.y() < min_y) blob_center.setY(min_y);
    if (blob_center.y() > max_y) blob_center.setY(max_y);
}

void BlobPhysics::ConstrainNeighborDistances(std::vector<QPointF>& control_points,
                                           std::vector<QPointF>& velocity,
                                           const double min_distance,
                                           const double max_distance) {

    if (control_points.empty()) return;

    const int num_of_points = control_points.size();

    for (int i = 0; i < num_of_points; ++i) {
        const int next = (i + 1) % num_of_points;

        QPointF difference = control_points[next] - control_points[i];
        const double distance = QVector2D(difference).length();

        if (distance < min_distance || distance > max_distance) {
            QPointF direction = difference / distance;
            const double target_distance = BlobMath::clamp(distance, min_distance, max_distance);

            const QPointF correction = direction * (distance - target_distance) * 0.5;
            control_points[i] += correction;
            control_points[next] -= correction;

            if (distance < min_distance) {
                velocity[i] -= direction * 0.3;
                velocity[next] += direction * 0.3;
            } else if (distance > max_distance) {
                velocity[i] += direction * 0.3;
                velocity[next] -= direction * 0.3;
            }
        }
    }
}

void BlobPhysics::SmoothBlobShape(std::vector<QPointF>& control_points) {
    if (control_points.empty()) return;

    const int num_of_points = control_points.size();
    std::vector<QPointF> smoothed_points = control_points;

    for (int i = 0; i < num_of_points; ++i) {
        const int prev = (i + num_of_points - 1) % num_of_points;
        const int next = (i + 1) % num_of_points;

        QPointF neighbor_average = (control_points[prev] + control_points[next]) * 0.5;

        smoothed_points[i] += (neighbor_average - control_points[i]) * 0.15;
    }

    control_points = smoothed_points;
}

void BlobPhysics::StabilizeBlob(std::vector<QPointF>& control_points,
                              const QPointF& blob_center,
                              const double blob_radius,
                              const double stabilization_rate) {
    const int num_of_points = control_points.size();

    double avg_distance = 0.0;
    for (int i = 0; i < num_of_points; ++i) {
        QPointF vector_from_center = control_points[i] - blob_center;
        avg_distance += QVector2D(vector_from_center).length();
    }
    avg_distance /= num_of_points;

    const double radius_ratio = blob_radius / avg_distance;

    static std::vector<double> random_offsets;
    if (random_offsets.size() != num_of_points) {
        random_offsets.resize(num_of_points);
        for (int i = 0; i < num_of_points; ++i) {
            random_offsets[i] = 0.95 + 0.1 * (qrand() % 100) / 100.0;
        }
    }

    for (int i = 0; i < num_of_points; ++i) {
        const double angle = 2 * M_PI * i / num_of_points;

        QPointF ideal_point(
            blob_center.x() + blob_radius * random_offsets[i] * qCos(angle),
            blob_center.y() + blob_radius * random_offsets[i] * qSin(angle)
        );

        QPointF vector_from_center = control_points[i] - blob_center;
        const double current_distance = QVector2D(vector_from_center).length();

        if (radius_ratio < 0.9 || radius_ratio > 1.1) {
            const double corrected_distance = current_distance * radius_ratio;
            if (current_distance > 0.001) {
                QPointF normalized_vector = vector_from_center * (1.0 / current_distance);
                control_points[i] = blob_center + normalized_vector * corrected_distance;
            }
        }

        control_points[i] += (ideal_point - control_points[i]) * stabilization_rate;
    }
}

bool BlobPhysics::ValidateAndRepairControlPoints(std::vector<QPointF>& control_points,
                                               std::vector<QPointF>& velocity,
                                               const QPointF& blob_center,
                                               const double blob_radius) {

    bool has_invalid_points = false;

    for (size_t i = 0; i < control_points.size(); ++i) {
        if (!BlobMath::isValidPoint(control_points[i]) || !BlobMath::isValidPoint(velocity[i])) {
            has_invalid_points = true;
            break;
        }
    }

    if (has_invalid_points) {
        const int num_of_points = control_points.size();

        for (int i = 0; i < num_of_points; ++i) {
            const double angle = 2 * M_PI * i / num_of_points;
            const double random_radius = blob_radius * (0.9 + 0.2 * (qrand() % 100) / 100.0);

            control_points[i] = QPointF(
                blob_center.x() + random_radius * qCos(angle),
                blob_center.y() + random_radius * qSin(angle)
            );

            velocity[i] = QPointF(0, 0);
        }
        return true;
    }

    return false;
}

QVector2D BlobPhysics::CalculateWindowVelocity(const QPointF& current_position) {
    double delta_time = physics_timer_.restart() / 1000.0;

    if (delta_time < 0.001) {
        delta_time = 0.016; // ~~60 FPS
    } else if (delta_time > 0.1) {
        delta_time = 0.1;  // min 10 FPS
    }

    QVector2D window_velocity(
        (current_position.x() - last_window_position_.x()) / delta_time,
        (current_position.y() - last_window_position_.y()) / delta_time
    );

    static QVector2D last_velocity(0, 0);

    window_velocity = window_velocity * 0.7 + last_velocity * 0.3;
    last_velocity = window_velocity;

    constexpr double max_velocity = 3000.0;
    if (window_velocity.length() > max_velocity) {
        window_velocity = window_velocity.normalized() * max_velocity;
    }

    last_window_velocity_ = window_velocity;

    return window_velocity;
}

void BlobPhysics::SetLastWindowPos(const QPointF& position) {
    last_window_position_ = position;
}

QPointF BlobPhysics::GetLastWindowPos() const {
    return last_window_position_;
}

QVector2D BlobPhysics::GetLastWindowVelocity() const {
    return last_window_velocity_;
}